use crate::platform::memory_layout::PAGE_FRAME_SIZE;
use crate::platform::physical_memory_manager::PhysicalMemoryManager;
use crate::platform::tasks::bindings::vmm_context;
use crate::platform::virtual_memory_manager_context::{
    VirtualMemoryManagerContext, VirtualMemoryMappingFlags,
};
use crate::platform::virtual_page_address::VirtualPageAddress;
use alloc::boxed::Box;
use alloc::sync::Arc;
use core::ffi::c_void;
use core::pin::Pin;

pub(super) mod bindings {
    include_bindings!("tasks.rs");
}

pub const TASK_KERNEL_STACK_SIZE: usize = 4 * PAGE_FRAME_SIZE;

#[derive(Debug, Copy, Clone)]
#[repr(transparent)]
pub struct TaskState(pub(super) *mut bindings::interrupt_frame);

unsafe impl Send for TaskState {}

#[derive(Debug)]
pub struct Task {
    #[allow(unused)]
    user_ctx: Option<Arc<VirtualMemoryManagerContext>>,
    kernel_stack: Pin<Box<[u8]>>,
    #[allow(unused)]
    user_stack: Option<Pin<Box<[u8]>>>,
    state: TaskState,
}

impl Task {
    pub fn new_user(user_ctx: Arc<VirtualMemoryManagerContext>, entrypoint_vaddr: usize) -> Self {
        let kernel_stack = unsafe {
            Pin::new_unchecked(Box::<[u8]>::new_zeroed_slice(TASK_KERNEL_STACK_SIZE).assume_init())
        };
        let user_stack = unsafe {
            Pin::new_unchecked(Box::<[u8]>::new_zeroed_slice(4 * PAGE_FRAME_SIZE).assume_init())
        };
        let user_stack_top_vaddr = 0x7FFFFFFFF000usize;
        for i in 0..4 {
            // allocate 4 pages as stack
            let page_vaddr = user_stack_top_vaddr - (i + 1) * PAGE_FRAME_SIZE;
            #[allow(mutable_transmutes)]
            unsafe {
                let page_phys = PhysicalMemoryManager::alloc_frame().unwrap();
                // @TODO: REPLACE THIS AWFUL CODE WITH PROBABLY ATOMICS OR SOMETHING
                let user_ctx: &VirtualMemoryManagerContext = user_ctx.as_ref();
                let user_ctx: &mut VirtualMemoryManagerContext = core::mem::transmute(user_ctx);
                user_ctx
                    .map_page(
                        VirtualPageAddress::new(page_vaddr).unwrap(),
                        page_phys,
                        VirtualMemoryMappingFlags::PRESENT
                            | VirtualMemoryMappingFlags::USER
                            | VirtualMemoryMappingFlags::WRITE,
                    )
                    .unwrap();
            }
        }

        let state = unsafe {
            let user_ctx_ptr: *const vmm_context = core::mem::transmute(user_ctx.inner());
            let kernel_stack_top = kernel_stack.as_ptr_range().end as usize;
            TaskState(bindings::task_setup_user(
                user_ctx_ptr,
                entrypoint_vaddr,
                user_stack_top_vaddr,
                kernel_stack_top,
            ))
        };

        Self {
            user_ctx: Some(user_ctx),
            user_stack: Some(user_stack),
            kernel_stack,
            state,
        }
    }

    pub fn new_kernel(
        function: unsafe extern "C" fn(arg: *mut c_void),
        arg: *mut c_void,
        kernel_stack_size: usize,
    ) -> Self {
        let kernel_stack = unsafe {
            Pin::new_unchecked(Box::<[u8]>::new_zeroed_slice(kernel_stack_size).assume_init())
        };

        let state = unsafe {
            let kernel_stack_top = kernel_stack.as_ptr_range().end as usize;
            TaskState(bindings::task_setup_kernel(kernel_stack_top, Some(function), arg))
        };

        Self {
            user_ctx: None,
            user_stack: None,
            kernel_stack,
            state,
        }
    }

    pub fn get_state(&self) -> TaskState {
        self.state
    }

    pub fn set_state(&mut self, state: TaskState) {
        self.state = state;
    }

    pub(super) unsafe fn prepare_switch(&self) {
        unsafe {
            let kernel_stack_top = self.kernel_stack.as_ptr_range().end as usize;
            bindings::task_prepare_switch(kernel_stack_top);
        }
    }
}
