use crate::platform::drivers::serial::SerialDriver;
use crate::platform::syscalls::{SyscallContext, SyscallIntent};
use crate::platform::tasks::TaskFrame;
use crate::syscall_handler::{SyscallCommand, SyscallCommandHandler, SyscallHandler};

pub struct SysExitCommand {
    task_frame: TaskFrame,
}

impl SyscallCommand for SysExitCommand {
    fn parse<'a>(ctx: &SyscallContext<'a>) -> Self
    where
        Self: 'a
    {
        Self {
            task_frame: *ctx.task_frame,
        }
    }
}

impl SyscallCommandHandler<SysExitCommand> for SyscallHandler {
    fn handle_command(&self, command: SysExitCommand) -> SyscallIntent {
        unsafe {
            SerialDriver::println("=== EXIT SYSCALL ===");
        }
        let next_task_state = self.scheduler.exit_current_task(command.task_frame).unwrap();

        SyscallIntent::SwitchTo(next_task_state)
    }
}
