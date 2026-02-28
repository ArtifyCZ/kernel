use crate::interrupt_safe_spin_lock::{InterruptSafeSpinLock, InterruptSafeSpinLockGuard};
use crate::platform::tasks::TaskContext;
use crate::task_id::TaskId;
use alloc::boxed::Box;
use alloc::collections::BTreeMap;
use core::ops::{Deref, DerefMut};

#[derive(Default)]
pub struct TaskRegistry(InterruptSafeSpinLock<TaskRegistryInner>);

#[derive(Default)]
struct TaskRegistryInner {
    tasks: BTreeMap<TaskId, TaskContext>,
}

impl TaskRegistry {
    pub fn new() -> &'static Self {
        Box::leak(Box::new(Default::default()))
    }

    pub fn get(&self, id: TaskId) -> Option<TaskGuard<'_>> {
        let inner = self.0.lock();
        if !inner.tasks.contains_key(&id) {
            return None;
        }
        Some(TaskGuard { id, inner })
    }

    pub fn insert(&self, id: TaskId, task: TaskContext) {
        let mut inner = self.0.lock();
        if inner.tasks.contains_key(&id) {
            panic!("Trying to insert a task with an existing id: {:?}!", id);
        }
        inner.tasks.insert(id, task);
    }
}

pub struct TaskGuard<'a> {
    id: TaskId,
    inner: InterruptSafeSpinLockGuard<'a, TaskRegistryInner>,
}

impl Deref for TaskGuard<'_> {
    type Target = TaskContext;

    fn deref(&self) -> &Self::Target {
        self.inner.tasks.get(&self.id).expect("Non-existence should be checked before creating TaskGuard!")
    }
}

impl DerefMut for TaskGuard<'_> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.inner.tasks.get_mut(&self.id).expect("Non-existence should be checked before creating TaskGuard!")
    }
}
