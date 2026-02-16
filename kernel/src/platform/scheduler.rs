use core::ffi::c_void;
use core::ptr::null_mut;
use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;

mod bindings {
    include_bindings!("scheduler.rs");
}

pub struct Scheduler;

impl Scheduler {
    pub unsafe fn init() {
        unsafe {
            bindings::sched_init();
        }
    }

    pub unsafe fn start() -> ! {
        loop {
            unsafe {
                bindings::sched_start();
            }
        }
    }

    pub unsafe fn create_user(user_ctx: &mut VirtualMemoryManagerContext, entrypoint_vaddr: usize) -> i32 {
        unsafe {
            let user_ctx = core::mem::transmute(user_ctx.inner_mut());
            bindings::sched_create_user(user_ctx, entrypoint_vaddr)
        }
    }

    pub unsafe fn create_kernel(function: unsafe extern "C" fn(args: *mut c_void)) -> i32 {
        unsafe {
            bindings::sched_create_kernel(Some(function), null_mut())
        }
    }
}
