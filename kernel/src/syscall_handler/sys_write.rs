use core::ptr::slice_from_raw_parts;
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::syscalls::{SyscallContext, SyscallIntent};
use crate::platform::terminal::Terminal;
use crate::syscall_handler::{SyscallCommand, SyscallCommandHandler, SyscallHandler};

pub struct SysWriteCommand {
    fd: i32,
    buf: *const u8,
    count: usize,
}

impl SyscallCommand for SysWriteCommand {
    fn parse<'a>(ctx: &SyscallContext<'a>) -> Option<Self>
    where
        Self: 'a
    {
        Some(Self {
            fd: ctx.args[0] as i32,
            buf: ctx.args[1] as *const u8,
            count: ctx.args[2] as usize,
        })
    }
}

impl SyscallCommandHandler<SysWriteCommand> for SyscallHandler {
    fn handle_command(&self, command: SysWriteCommand) -> SyscallIntent {
        // @TODO: add defensive checks (e.g. is the buffer in its full size mapped to the address space?
        // @TODO: use new-type and other patterns
        // stdout or stderr
        if command.fd != 1 && command.fd != 2 {
            // EBADF: Bad File Descriptor
            return SyscallIntent::Return(1);
        }

        // Basic Range Check: Is the buffer in User Space?
        // On x86_64, user addresses are usually < 0x00007FFFFFFFFFFF
        if command.buf as usize >= 0x800000000000usize || (command.buf as usize + command.count) >= 0x800000000000 {
            // EFAULT: Bad Address
            return SyscallIntent::Return(1);
        }

        unsafe {
            let user_buf = slice_from_raw_parts(command.buf, command.count)
                .as_ref()
                .unwrap();
            SerialDriver::write(user_buf);
            Terminal::print_bytes(user_buf);
        }

        SyscallIntent::Return(0)
    }
}
