use crate::platform::drivers::serial::SerialDriver;
use crate::platform::scheduler::bindings::{interrupts_disable, interrupts_enable};
use crate::platform::tasks::Task;
use alloc::boxed::Box;
use alloc::vec::Vec;

mod bindings {
    include_bindings!("scheduler.rs");
}

#[repr(C)]
pub struct Scheduler {
    current_task: i32,
    started: bool,
    tasks: Vec<Task>,
}

impl Default for Scheduler {
    fn default() -> Self {
        Scheduler {
            current_task: -1,
            started: false,
            tasks: Vec::with_capacity(16),
        }
    }
}

static mut SCHEDULER: Option<Box<Scheduler>> = None;

impl Scheduler {
    pub unsafe fn init() {
        unsafe {
            SerialDriver::println("Initializing scheduler...");
            SCHEDULER = Some(Default::default());
            SerialDriver::println("Scheduler initialized!");
        }
    }

    #[allow(static_mut_refs)]
    pub unsafe fn start() -> ! {
        loop {
            unsafe {
                let scheduler = SCHEDULER.as_mut().unwrap().as_mut();
                scheduler.started = true;
                bindings::sched_start();
            }
        }
    }

    fn find_next_runnable_task(&self) -> Option<(usize, &Task)> {
        for offset in 1..=self.tasks.len() {
            let idx = if self.current_task < 0 {
                offset - 1
            } else {
                ((self.current_task as usize) + offset) % self.tasks.len()
            };
            return Some((idx, &self.tasks[idx]));
        }

        None
    }

    pub fn add(&mut self, task: Task) {
        self.tasks.push(task);
    }

    #[allow(static_mut_refs)]
    pub fn static_add(task: Task) {
        unsafe {
            interrupts_disable();
            let scheduler = SCHEDULER.as_mut().unwrap().as_mut();
            scheduler.add(task);
            interrupts_enable();
        }
    }

    #[allow(static_mut_refs)]
    pub unsafe fn thread_exit(prev_thread_ctx: *mut super::tasks::bindings::interrupt_frame) -> *mut super::tasks::bindings::interrupt_frame {
        unsafe {
            interrupts_disable();
            // @TODO: implement exit on tasks
            Self::heartbeat(prev_thread_ctx)
        }
    }

    #[allow(static_mut_refs)]
    pub(super) unsafe fn heartbeat(prev_thread_ctx: *mut super::tasks::bindings::interrupt_frame) -> *mut super::tasks::bindings::interrupt_frame {
        unsafe {
            interrupts_disable();
            let scheduler = match SCHEDULER.as_mut() {
                None => {
                    interrupts_enable();
                    return prev_thread_ctx;
                }
                Some(scheduler) => scheduler.as_mut(),
            };
            if !scheduler.started {
                interrupts_enable();
                return prev_thread_ctx;
            }

            let prev_idx = scheduler.current_task;

            let (next_idx, next_thread) = match scheduler.find_next_runnable_task() {
                None => {
                    interrupts_enable();
                    return prev_thread_ctx;
                }
                Some((next_idx, next_thread)) => (next_idx as i32, next_thread),
            };

            if next_idx == prev_idx {
                interrupts_enable();
                return prev_thread_ctx;
            }

            next_thread.prepare_switch();
            let next_thread_ctx = next_thread.get_interrupt_frame();

            if prev_idx >= 0 {
                let prev_task = &mut scheduler.tasks[prev_idx as usize];
                prev_task.set_interrupt_frame(prev_thread_ctx);
            }

            scheduler.current_task = next_idx;

            interrupts_enable();
            next_thread_ctx
        }
    }
}
