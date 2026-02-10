#![no_std]
#![no_main]

extern crate alloc;

mod allocator;
mod entrypoint;
mod platform;
mod spin_lock;

use alloc::ffi::CString;
use core::ffi::c_char;
use core::str::FromStr;

#[panic_handler]
#[cfg(not(test))]
fn panic(_: &::core::panic::PanicInfo) -> ! {
    unsafe {
        serial_println(b"Panic occurred; cannot print the message yet\0".as_ptr() as *const c_char);
        hcf()
    }
}

unsafe extern "C" {

    /// Halt and catch fire; implemented in C
    fn hcf() -> !;
}

pub(crate) use platform::serial_println;

fn main() {
    unsafe {
        let message = CString::from_str("Hello from CString in Rust!").expect("Failed to create CString");
        serial_println(message.as_ptr());
    }
}
