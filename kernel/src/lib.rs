#![no_std]
#![no_main]

use core::ffi::c_char;
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
        let message = b"Hello from Rust!\0";
        serial_println(message.as_ptr() as *const c_char);
    }
}
