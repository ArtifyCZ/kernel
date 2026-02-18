use core::ffi::c_void;
use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;

pub(super) mod bindings {
    include_bindings!("tasks.rs");
}

pub struct Task;

impl Task {
    pub unsafe fn setup_user(
        user_ctx: &VirtualMemoryManagerContext,
        entrypoint_vaddr: usize,
        user_stack_top: usize,
        kernel_stack_top: usize,
    ) -> *mut bindings::interrupt_frame {
        unsafe {
            bindings::task_setup_user(
                core::mem::transmute(user_ctx.inner()),
                entrypoint_vaddr,
                user_stack_top,
                kernel_stack_top,
            )
        }
    }

    pub unsafe fn setup_kernel(
        stack_top: usize,
        function: unsafe extern "C" fn(arg: *mut c_void),
        arg: *mut c_void,
    ) -> *mut bindings::interrupt_frame {
        unsafe {
            bindings::task_setup_kernel(stack_top, Some(function), arg)
        }
    }

    pub unsafe fn prepare_switch(kernel_stack_top: usize) {
        unsafe {
            bindings::task_prepare_switch(kernel_stack_top);
        }
    }
}
