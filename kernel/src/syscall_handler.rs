use crate::platform::drivers::serial::SerialDriver;
use crate::platform::memory_layout::PAGE_FRAME_SIZE;
use crate::platform::physical_memory_manager::PhysicalMemoryManager;
use crate::platform::syscalls::{syscall_num, SyscallContext, SyscallIntent};
use crate::platform::terminal::Terminal;
use crate::platform::virtual_memory_manager_context::VirtualMemoryMappingFlags;
use crate::platform::virtual_page_address::VirtualPageAddress;
use crate::scheduler::Scheduler;
use crate::task_registry::TaskSpec;
use alloc::boxed::Box;
use alloc::format;
use core::ptr::slice_from_raw_parts;

pub struct SyscallHandler {
    scheduler: &'static Scheduler,
}

impl SyscallHandler {
    pub fn init(scheduler: &'static Scheduler) -> &'static Self {
        let syscall_handler: &'static Self = Box::leak(Box::new(SyscallHandler { scheduler }));
        syscall_handler
    }

    pub fn handle(&self, ctx: &SyscallContext<'_>) -> SyscallIntent {
        match ctx.num {
            syscall_num::SYS_EXIT => self.sys_exit(ctx),
            syscall_num::SYS_WRITE => self.sys_write(ctx),
            syscall_num::SYS_CLONE => self.sys_clone(ctx),
            syscall_num::SYS_MMAP => self.sys_mmap(ctx),
            _ => panic!("Non-existent syscall triggered!"), // @TODO: add better handling
        }
    }

    fn sys_exit(&self, ctx: &SyscallContext<'_>) -> SyscallIntent {
        unsafe {
            SerialDriver::println("=== EXIT SYSCALL ===");
        }
        let prev_task_state = ctx.task_frame.clone();
        let next_task_state = self.scheduler.exit_current_task(prev_task_state).unwrap();

        SyscallIntent::SwitchTo(next_task_state)
    }

    fn sys_write(&self, ctx: &SyscallContext<'_>) -> SyscallIntent {
        let fd = ctx.args[0];
        let user_buf = ctx.args[1];
        let count = ctx.args[2];

        // stdout or stderr
        if fd != 1 && fd != 2 {
            // EBADF: Bad File Descriptor
            return SyscallIntent::Return(1);
        }

        // Basic Range Check: Is the buffer in User Space?
        // On x86_64, user addresses are usually < 0x00007FFFFFFFFFFF
        if user_buf >= 0x800000000000 || (user_buf + count) >= 0x800000000000 {
            // EFAULT: Bad Address
            return SyscallIntent::Return(1);
        }

        let user_buf = user_buf as *const u8;
        unsafe {
            let user_buf = slice_from_raw_parts(user_buf, count as usize)
                .as_ref()
                .unwrap();
            SerialDriver::write(user_buf);
            Terminal::print_bytes(user_buf);
        }

        SyscallIntent::Return(0)
    }

    fn sys_clone(&self, ctx: &SyscallContext<'_>) -> SyscallIntent {
        let vmm = self
            .scheduler
            .access_current_task_context(|task| task.get_virtual_memory_manager().clone())
            .expect("Scheduler is not started yet!");
        // @TODO: implement flags
        let _flags = ctx.args[0];
        let stack_pointer = ctx.args[1] as usize;
        let entrypoint = ctx.args[2] as usize;
        if stack_pointer >= 0x800000000000 || entrypoint >= 0x800000000000 {
            return SyscallIntent::Return(0);
        }
        let pid = self.scheduler.spawn(TaskSpec::User {
            virtual_memory_manager_context: vmm,
            user_stack_vaddr: stack_pointer,
            entrypoint_vaddr: entrypoint,
        });
        SyscallIntent::Return(pid.get())
    }

    fn sys_mmap(&self, ctx: &SyscallContext<'_>) -> SyscallIntent {
        let addr = ctx.args[0] as usize;
        let length = ctx.args[1] as usize;
        let _prot = ctx.args[2] as u32;
        let _flags = ctx.args[3] as u32;

        if addr >= 0x800000000000 || (addr + length) >= 0x800000000000 {
            unsafe {
                SerialDriver::println("mmap: EFAULT: Bad Address");
                SerialDriver::println(&format!("addr: {}; len: {}", addr, length));
            }
            return SyscallIntent::Return(0);
        }

        const PAGE_MASK: usize = !(PAGE_FRAME_SIZE - 1);
        let addr = addr & PAGE_MASK;
        let pages_count = (length + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;
        // @TODO: implement protection flags
        // @TODO: implement flags

        for page_idx in 0..pages_count {
            let page_vaddr = VirtualPageAddress::new(addr + page_idx * PAGE_FRAME_SIZE).unwrap();
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

        SyscallIntent::Return(addr as u64)
    }
}
