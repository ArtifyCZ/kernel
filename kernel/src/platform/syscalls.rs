use alloc::ffi::CString;
use core::ptr::slice_from_raw_parts;
use crate::hcf;
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::terminal::Terminal;

mod bindings {
    include_bindings!("syscalls.rs");
}

pub struct Syscalls;

impl Syscalls {
    pub unsafe fn init() {
        unsafe {
            SerialDriver::println("Initializing syscalls...");
            bindings::syscalls_init();
            SerialDriver::println("Syscalls initialized!");
        }
    }
}

fn sys_exit() -> u64 {
    unsafe {
        SerialDriver::println("=== EXIT SYSCALL ===");
        hcf()
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
        let user_buf = slice_from_raw_parts(user_buf, count as usize).as_ref().unwrap();
        SerialDriver::write(user_buf);
        Terminal::print_bytes(user_buf);
    }
    0
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn syscalls_dispatch(frame: *mut bindings::syscall_frame) -> u64 {
    let frame = unsafe { frame.as_mut() }.unwrap();
    match frame.num {
        0x00 => sys_exit(),
        0x01 => sys_write(frame),
        _ => panic!("Non-existent syscall triggered!"), // @TODO: add better handling
    }
}
