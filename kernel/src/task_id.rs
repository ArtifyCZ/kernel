use core::sync::atomic::AtomicU64;
use crate::platform::syscalls::{SyscallReturnValue, SyscallReturnable};

/// Each task has a unique ID.
///
/// This id is guaranteed to be completely unique across CPU cores.
#[derive(Copy, Clone, Debug, Ord, PartialOrd, Eq, PartialEq, Hash)]
pub struct TaskId(u64);

static NEXT_ID: AtomicU64 = AtomicU64::new(0);

impl TaskId {
    pub fn new() -> Self {
        let id = NEXT_ID.fetch_add(1, core::sync::atomic::Ordering::Relaxed);
        Self(id)
    }

    pub fn get(&self) -> u64 {
        self.0
    }
}

impl SyscallReturnable for TaskId {
    fn into_return_value(self) -> SyscallReturnValue {
        SyscallReturnValue(self.get())
    }
}

impl From<u64> for TaskId {
    fn from(value: u64) -> Self {
        Self(value)
    }
}
