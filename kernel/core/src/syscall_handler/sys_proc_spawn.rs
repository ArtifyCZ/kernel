use crate::platform::syscalls::{SyscallContext, SyscallError, SyscallIntent};
use crate::syscall_handler::user_ptr::UserPtr;
use crate::syscall_handler::{SyscallCommand, SyscallCommandHandler, SyscallHandler};
use crate::task_id::TaskId;
use crate::task_registry::TaskSpec;

pub struct SysProcSpawnCommand {
    proc_handle: u64,
    // @TODO: implement flags
    #[allow(unused)]
    flags: u64,
    stack_pointer: UserPtr<usize>,
    entrypoint: UserPtr<usize>,
    arg: u64,
}

impl SyscallCommand for SysProcSpawnCommand {
    type Error = SyscallError;

    fn parse<'a>(ctx: &SyscallContext<'a>) -> Result<Self, Self::Error>
    where
        Self: 'a,
    {
        let proc_handle = ctx.args[0];
        let flags = ctx.args[1];
        let stack_pointer = UserPtr::try_from(ctx.args[2])?;
        let entrypoint = UserPtr::try_from(ctx.args[3])?;
        let arg = ctx.args[4];

        Ok(Self {
            proc_handle,
            flags,
            stack_pointer,
            entrypoint,
            arg,
        })
    }
}

impl SyscallCommandHandler<SysProcSpawnCommand> for SyscallHandler {
    type Ok = TaskId;
    type Err = SyscallError;

    fn handle_command(
        &self,
        command: SysProcSpawnCommand,
    ) -> Result<SyscallIntent<Self::Ok>, Self::Err> {
        let current_task_id = TaskId::get_current().expect("Scheduler is not started yet!");
        let vmm = {
            let task = self
                .task_registry
                .get(current_task_id)
                .expect("Current task should exist!");
            if command.proc_handle == 0 {
                task.get_virtual_memory_manager().clone()
            } else {
                task.get_proc_handle(command.proc_handle)
                    .expect("Not assigned proc handle not handled")
                    .clone()
            }
        };

        let pid = self.scheduler.spawn(TaskSpec::User {
            virtual_memory_manager_context: vmm,
            user_stack_vaddr: *command.stack_pointer,
            entrypoint_vaddr: *command.entrypoint,
            arg: command.arg,
        });
        Ok(SyscallIntent::Return(pid))
    }
}
