#![no_std]
#![no_main]

extern crate alloc;

mod allocator;
mod platform;

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

#[unsafe(no_mangle)]
pub unsafe extern "C" fn kernel_main() {
    main();
}

unsafe extern "C" {
    fn serial_println(message: *const c_char);

    /// Halt and catch fire; implemented in C
    fn hcf() -> !;
}

fn main() {
    unsafe {
        let message = CString::from_str("Hello from CString in Rust!").expect("Failed to create CString");
        serial_println(message.as_ptr());
    }
}
