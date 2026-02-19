use crate::platform::drivers::serial::SerialDriver;
use crate::platform::scheduler::Scheduler;
use crate::platform::syscalls::bindings::syscall_frame;
use crate::platform::terminal::Terminal;
use core::ffi::c_void;
use core::ptr::{null_mut, slice_from_raw_parts};
use crate::platform::tasks::TaskState;

mod bindings {
    include_bindings!("syscalls.rs");
}

pub struct Syscalls;

impl Syscalls {
    pub unsafe fn init() {
        unsafe {
            SerialDriver::println("Initializing syscalls...");
            bindings::syscalls_init(Some(Self::syscalls_dispatch), null_mut());
            SerialDriver::println("Syscalls initialized!");
        }
    }

    fn sys_exit(frame: &mut syscall_frame) -> u64 {
        unsafe {
            SerialDriver::println("=== EXIT SYSCALL ===");
            let prev_task_interrupt_frame = (*frame.interrupt_frame).cast();
            let prev_task_state = TaskState(prev_task_interrupt_frame);
            let next_task_state = Scheduler::task_exit(prev_task_state);
            let next_task_interrupt_frame = next_task_state.0;
            *frame.interrupt_frame = next_task_interrupt_frame.cast();
            0
        }
    }

    fn sys_write(frame: &mut bindings::syscall_frame) -> u64 {
        let fd = frame.a[0];
        let user_buf = frame.a[1];
        let count = frame.a[2];

        // stdout or stderr
        if fd != 1 && fd != 2 {
            return 1; // EBADF: Bad File Descriptor
        }

        // Basic Range Check: Is the buffer in User Space?
        // On x86_64, user addresses are usually < 0x00007FFFFFFFFFFF
        if user_buf >= 0x800000000000 || (user_buf + count) >= 0x800000000000 {
            return 1; // EFAULT: Bad Address
        }

        let user_buf = user_buf as *const u8;
        unsafe {
            let user_buf = slice_from_raw_parts(user_buf, count as usize)
                .as_ref()
                .unwrap();
            SerialDriver::write(user_buf);
            Terminal::print_bytes(user_buf);
        }
        0
    }

    unsafe extern "C" fn syscalls_dispatch(
        frame: *mut bindings::syscall_frame,
        _arg: *mut c_void,
    ) -> u64 {
        let frame = unsafe { frame.as_mut() }.unwrap();
        match frame.num {
            0x00 => Self::sys_exit(frame),
            0x01 => Self::sys_write(frame),
            _ => panic!("Non-existent syscall triggered!"), // @TODO: add better handling
        }
    }
}
