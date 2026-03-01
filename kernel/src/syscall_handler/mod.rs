mod sys_clone;
mod sys_exit;
mod sys_mmap;
mod sys_write;
mod user_ptr;
mod user_slice;

use crate::platform::syscalls::{
    SyscallContext, SyscallError, SyscallIntent, SyscallReturnValue, SyscallReturnable, syscall_num,
};
use crate::scheduler::Scheduler;
use crate::syscall_handler::sys_clone::SysCloneCommand;
use crate::syscall_handler::sys_exit::SysExitCommand;
use crate::syscall_handler::sys_mmap::SysMmapCommand;
use crate::syscall_handler::sys_write::SysWriteCommand;
use alloc::boxed::Box;

pub struct SyscallHandler {
    scheduler: &'static Scheduler,
}

pub trait SyscallCommand: Sized {
    type Error: Into<SyscallError>;

    fn parse<'a>(ctx: &SyscallContext<'a>) -> Result<Self, Self::Error>
    where
        Self: 'a;
}

pub trait SyscallCommandHandler<TSyscallCommand> {
    type Ok: SyscallReturnable;
    type Err: Into<SyscallError>;

    fn handle_command(
        &self,
        command: TSyscallCommand,
    ) -> Result<SyscallIntent<Self::Ok>, Self::Err>;
}

impl SyscallHandler {
    pub fn init(scheduler: &'static Scheduler) -> &'static Self {
        let syscall_handler: &'static Self = Box::leak(Box::new(SyscallHandler { scheduler }));
        syscall_handler
    }

    pub fn handle(
        &self,
        ctx: &SyscallContext<'_>,
    ) -> Result<SyscallIntent<SyscallReturnValue>, SyscallError> {
        Ok(match ctx.num {
            syscall_num::SYS_EXIT => self.handle_command(SysExitCommand::parse(ctx)?)?.into(),
            syscall_num::SYS_WRITE => self.handle_command(SysWriteCommand::parse(ctx)?)?.into(),
            syscall_num::SYS_CLONE => self.handle_command(SysCloneCommand::parse(ctx)?)?.into(),
            syscall_num::SYS_MMAP => self.handle_command(SysMmapCommand::parse(ctx)?)?.into(),
            _ => return Err(SyscallError::SYS_ENOSYS),
        })
    }
}
