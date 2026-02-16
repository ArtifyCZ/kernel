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
}
