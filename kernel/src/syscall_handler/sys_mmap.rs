use alloc::format;
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::memory_layout::PAGE_FRAME_SIZE;
use crate::platform::physical_memory_manager::PhysicalMemoryManager;
use crate::platform::syscalls::{SyscallContext, SyscallError, SyscallIntent};
use crate::platform::virtual_memory_manager_context::VirtualMemoryMappingFlags;
use crate::platform::virtual_page_address::VirtualPageAddress;
use crate::syscall_handler::{SyscallCommand, SyscallCommandHandler, SyscallHandler};
use crate::syscall_handler::user_ptr::UserPtr;
use crate::syscall_handler::user_slice::UserSlice;

pub struct SysMmapCommand {
    chunk: UserSlice<*const [u8]>,
    // @TODO: implement protection flags
    #[allow(unused)]
    prot: u32,
    // @TODO: implement flags
    #[allow(unused)]
    flags: u32,
}

impl SyscallCommand for SysMmapCommand {
    type Error = SyscallError;

    fn parse<'a>(ctx: &SyscallContext<'a>) -> Result<Self, Self::Error>
    where
        Self: 'a
    {
        let addr = ctx.args[0] as usize;
        let length = ctx.args[1] as usize;
        let prot = ctx.args[2] as u32;
        let flags = ctx.args[3] as u32;

        let chunk_start = UserPtr::try_from(addr)?;
        let chunk = UserSlice::try_from((chunk_start, length))?;

        Ok(Self {
            chunk,
            prot,
            flags,
        })
    }
}

impl SyscallCommandHandler<SysMmapCommand> for SyscallHandler {
    type Ok = UserPtr<usize>;
    type Err = SyscallError;

    fn handle_command(&self, command: SysMmapCommand) -> Result<SyscallIntent<Self::Ok>, Self::Err> {
        const PAGE_MASK: usize = !(PAGE_FRAME_SIZE - 1);
        let addr = UserPtr::try_from(command.chunk.addr() & PAGE_MASK)?;
        let pages_count = (command.chunk.len() + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;

        for page_idx in 0..pages_count {
            let page_vaddr = UserPtr::try_from(*addr + page_idx * PAGE_FRAME_SIZE)?;
            let page_vaddr = VirtualPageAddress::new(*page_vaddr).unwrap();
            let phys = unsafe { PhysicalMemoryManager::alloc_frame() }.unwrap();
            unsafe {
                self.scheduler.access_current_task_context(|task| {
                    task.get_virtual_memory_manager().map_page(
                        page_vaddr,
                        phys,
                        VirtualMemoryMappingFlags::PRESENT
                            | VirtualMemoryMappingFlags::USER
                            | VirtualMemoryMappingFlags::WRITE,
                    )
                });
            }
        }

        Ok(SyscallIntent::Return(addr))
    }
}
