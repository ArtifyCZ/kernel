use crate::platform::syscalls::{SyscallContext, SyscallError, SyscallIntent};
use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;
use crate::syscall_handler::{SyscallCommand, SyscallCommandHandler, SyscallHandler};
use crate::task_id::TaskId;
use alloc::sync::Arc;

pub struct SysProcCreateCommand {
    #[allow(unused)]
    flags: u64,
}

impl SyscallCommand for SysProcCreateCommand {
    type Error = SyscallError;

    fn parse<'a>(ctx: &SyscallContext<'a>) -> Result<Self, Self::Error>
    where
        Self: 'a,
    {
        let flags = ctx.args[0];

        Ok(Self { flags })
    }
}

impl SyscallCommandHandler<SysProcCreateCommand> for SyscallHandler {
    type Ok = u64;
    type Err = SyscallError;

    fn handle_command(
        &self,
        #[allow(unused)]
        command: SysProcCreateCommand,
    ) -> Result<SyscallIntent<Self::Ok>, Self::Err> {
        let task_id = TaskId::get_current().expect("Scheduler is not started yet!");
        let mut task = self
            .task_registry
            .get(task_id)
            .expect("Current task should exist!");
        let vmm = Arc::new(unsafe { VirtualMemoryManagerContext::create() });
        let handle = task.add_proc_handle(vmm);
        Ok(SyscallIntent::Return(handle))
    }
}
