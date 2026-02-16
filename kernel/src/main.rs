#![no_std]
#![no_main]

extern crate alloc;

mod allocator;
mod entrypoint;
mod platform;
mod spin_lock;

use alloc::ffi::CString;
use core::ffi::{c_char, CStr};
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
use crate::platform::elf::Elf;
use crate::platform::modules::Modules;
use crate::platform::scheduler::Scheduler;
use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;

fn main() {
    unsafe {
        let message = CString::from_str("Hello from CString in Rust!").expect("Failed to create CString");
        serial_println(message.as_ptr());

        let init_elf_string = CString::from_str("init.elf").expect("Failed to create CString");
        let init_elf = Modules::find(init_elf_string.as_c_str()).unwrap();
        let mut init_ctx = VirtualMemoryManagerContext::create();
        let entrypoint_vaddr = Elf::load(&mut init_ctx, init_elf).unwrap();
        Scheduler::create_user(&mut init_ctx, entrypoint_vaddr);

        Scheduler::start();
    }
}
