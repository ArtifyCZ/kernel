use crate::interrupt_safe_spin_lock::InterruptSafeSpinLock;
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::tasks::{TaskContext, TaskFrame};
use crate::task_id::TaskId;
use crate::task_registry::{TaskGuard, TaskRegistry};
use alloc::boxed::Box;
use alloc::collections::VecDeque;
use core::ops::{Deref, DerefMut};

#[derive(Default)]
pub struct Scheduler(InterruptSafeSpinLock<SchedulerInner>);

#[repr(C)]
struct SchedulerInner {
    current_task: Option<TaskId>,
    null_task: TaskId,
    started: bool,
    tasks: &'static TaskRegistry,
    ready_tasks: VecDeque<TaskId>,
}

impl Default for SchedulerInner {
    fn default() -> Self {
        let null_task_id = TaskId::new();
        let null_task = TaskContext::new_kernel_null();
        let tasks = TaskRegistry::new();
        tasks.insert(null_task_id, null_task);
        let mut ready_tasks = VecDeque::new();
        ready_tasks.push_back(null_task_id);
        SchedulerInner {
            started: false,
            current_task: None,
            null_task: null_task_id,
            tasks,
            ready_tasks,
        }
    }
}

impl SchedulerInner {
    fn pick_next_task(&mut self) -> Option<TaskGuard<'_>> {
        if !self.started {
            return None;
        }

        while let Some(task_id) = self.ready_tasks.pop_front() {
            if let Some(task) = self.tasks.get(task_id) {
                self.current_task = Some(task_id);
                return Some(task);
            }
        }

        None
    }
}

impl Scheduler {
    pub fn init(task_registry: &'static TaskRegistry) -> &'static Self {
        unsafe {
            SerialDriver::println("Initializing scheduler...");
            let null_task_id = TaskId::new();
            let null_task = TaskContext::new_kernel_null();
            task_registry.insert(null_task_id, null_task);
            let mut ready_tasks = VecDeque::new();
            ready_tasks.push_back(null_task_id);
            let scheduler: &'static Self = Box::leak(Box::new(Scheduler(
                InterruptSafeSpinLock::new(SchedulerInner {
                    started: false,
                    current_task: None,
                    null_task: null_task_id,
                    tasks: task_registry,
                    ready_tasks,
                }),
            )));
            SerialDriver::println("Scheduler initialized!");
            scheduler
        }
    }

    pub fn start(&self) {
        let mut inner = self.0.lock();
        inner.started = true;
    }

    pub fn add(&self, task: TaskContext) {
        let mut inner = self.0.lock();
        let task_id = TaskId::new();
        inner.tasks.insert(task_id, task);
        inner.ready_tasks.push_back(task_id);
    }

    pub fn update_current_task_context(&self, f: impl FnOnce(&mut TaskContext)) {
        let mut inner = self.0.lock();
        if !inner.started {
            return;
        }
        let task_id = match inner.current_task {
            Some(task_id) => task_id,
            None => return,
        };
        let mut task = inner.tasks.get(task_id).unwrap();
        f(task.deref_mut());
    }

    pub fn access_current_task_context<TOut>(
        &self,
        f: impl FnOnce(&TaskContext) -> TOut,
    ) -> Option<TOut> {
        let inner = self.0.lock();
        if !inner.started {
            return None;
        }
        let task_id = inner.current_task?;
        let task = inner.tasks.get(task_id).unwrap();
        Some(f(task.deref()))
    }

    pub fn heartbeat(&self, prev_frame: TaskFrame) -> Option<TaskFrame> {
        let mut inner = self.0.lock();
        if !inner.started {
            return None;
        }

        if let Some(prev_idx) = inner.current_task {
            let mut prev_task = inner.tasks.get(prev_idx).unwrap();
            prev_task.set_frame(prev_frame);
            inner.ready_tasks.push_back(prev_idx);
        }

        let next_task = inner.pick_next_task()?;
        Some(next_task.prepare_switch())
    }

    pub fn exit_current_task(&self, prev_frame: TaskFrame) -> Option<TaskFrame> {
        let mut inner = self.0.lock();
        if !inner.started {
            return None;
        }

        if let Some(prev_id) = inner.current_task {
            let mut prev_task = inner.tasks.get(prev_id).unwrap();
            prev_task.set_frame(prev_frame);
            if let Some((idx, _)) = inner
                .ready_tasks
                .iter()
                .enumerate()
                .find(|(_, task_id)| **task_id == prev_id)
            {
                inner.ready_tasks.remove(idx);
            }
        }

        let next_task = inner.pick_next_task()?;
        Some(next_task.prepare_switch())
    }
}
