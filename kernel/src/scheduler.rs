use crate::interrupt_safe_spin_lock::InterruptSafeSpinLock;
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::tasks::TaskContext;
use crate::task_id::TaskId;
use alloc::boxed::Box;
use alloc::collections::BTreeMap;

#[derive(Default)]
pub struct Scheduler(InterruptSafeSpinLock<SchedulerInner>);

#[repr(C)]
struct SchedulerInner {
    current_task: Option<TaskId>,
    null_task: TaskId,
    started: bool,
    tasks: BTreeMap<TaskId, TaskContext>,
}

impl Default for SchedulerInner {
    fn default() -> Self {
        let null_task_id = TaskId::new();
        let null_task = TaskContext::new_kernel_null();
        let mut tasks = BTreeMap::new();
        tasks.insert(null_task_id, null_task);
        SchedulerInner {
            started: false,
            current_task: None,
            null_task: null_task_id,
            tasks,
        }
    }
}

impl SchedulerInner {
    fn pick_next_task(&mut self) -> Option<&mut TaskContext> {
        if !self.started {
            return None;
        }

        let tasks_count = self.tasks.len() as u64;
        for offset in 1..=tasks_count {
            let idx: TaskId = ((if let Some(current_id) = self.current_task {
                current_id.get() + offset
            } else {
                0
            }) % tasks_count)
                .into();
            self.current_task = Some(idx);
            if self.tasks.contains_key(&idx) {
                return self.tasks.get_mut(&idx);
            }
        }

        unreachable!("There should at the very least be the null task")
    }
}

impl Scheduler {
    pub fn init() -> &'static Self {
        unsafe {
            SerialDriver::println("Initializing scheduler...");
            let scheduler: &'static Self = Box::leak(Box::new(Default::default()));
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
        let task = inner.tasks.get_mut(&task_id).unwrap();
        f(task);
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
        Some(f(&inner.tasks[&task_id]))
    }

    pub fn heartbeat<FPrev, FNext, TOut>(&self, f_prev: FPrev, f_next: FNext) -> Option<TOut>
    where
        FPrev: FnOnce(&mut TaskContext),
        FNext: FnOnce(&mut TaskContext) -> TOut,
    {
        let mut inner = self.0.lock();
        if !inner.started {
            return None;
        }

        if let Some(prev_idx) = inner.current_task {
            let prev_task = inner.tasks.get_mut(&prev_idx).unwrap();
            f_prev(prev_task);
        }

        let next_task = inner.pick_next_task()?;
        let result = f_next(next_task);

        Some(result)
    }

    pub fn exit_current_task<FPrev, FNext, TOut>(
        &self,
        f_prev: FPrev,
        f_next: FNext,
    ) -> Option<TOut>
    where
        FPrev: FnOnce(&mut TaskContext),
        FNext: FnOnce(&mut TaskContext) -> TOut,
    {
        let mut inner = self.0.lock();
        if !inner.started {
            return None;
        }

        if let Some(prev_id) = inner.current_task {
            let prev_task = inner.tasks.get_mut(&prev_id).unwrap();
            f_prev(prev_task);
            inner.tasks.remove(&prev_id);
            let new_prev_id = TaskId::from(if prev_id.get() == 0 {
                inner.tasks.len() as u64 - 1
            } else {
                prev_id.get() - 1
            });
            inner.current_task = Some(new_prev_id);
        }

        let next_task = inner.pick_next_task().unwrap();
        let result = f_next(next_task);

        Some(result)
    }
}
