use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;

mod bindings {
    include_bindings!("scheduler.rs");
}

pub struct Scheduler;

impl Scheduler {
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
}
