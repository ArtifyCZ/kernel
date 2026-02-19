use crate::interrupt_safe_spin_lock::{InterruptSafeSpinLock, InterruptSafeSpinLockGuard};
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::tasks::{Task, TaskState};
use alloc::boxed::Box;
use alloc::vec::Vec;

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

static mut SCHEDULER: Option<&'static InterruptSafeSpinLock<Scheduler>> = None;

impl Scheduler {
    pub unsafe fn init() {
        unsafe {
            SerialDriver::println("Initializing scheduler...");
            SCHEDULER = Some(Box::leak(Box::new(Default::default())));
            SerialDriver::println("Scheduler initialized!");
        }
    }

    #[allow(static_mut_refs)]
    pub unsafe fn get_instance() -> InterruptSafeSpinLockGuard<'static, Self> {
        unsafe {
            SCHEDULER.as_ref().unwrap().lock()
        }
    }

    pub fn start(&mut self) {
        self.started = true;
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

    pub fn exit_task(&mut self, prev_task_state: TaskState) -> TaskState {
        unsafe {
            // @TODO: implement exit on tasks
            self.heartbeat(prev_task_state)
        }
    }

    #[allow(static_mut_refs)]
    pub(super) unsafe fn heartbeat(&mut self, prev_task_state: TaskState) -> TaskState {
        unsafe {
            if !self.started {
                return prev_task_state;
            }

            let prev_idx = self.current_task;

            let (next_idx, next_task) = match self.find_next_runnable_task() {
                None => {
                    return prev_task_state;
                }
                Some((next_idx, next_task)) => (next_idx as i32, next_task),
            };

            if next_idx == prev_idx {
                return prev_task_state;
            }

            next_task.prepare_switch();
            let next_task_state = next_task.get_state();

            if prev_idx >= 0 {
                let prev_task = &mut self.tasks[prev_idx as usize];
                prev_task.set_state(prev_task_state);
            }

            self.current_task = next_idx;

            next_task_state
        }
    }
}
