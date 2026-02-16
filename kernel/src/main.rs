#![no_std]
#![no_main]

extern crate alloc;

mod allocator;
mod entrypoint;
mod platform;
mod spin_lock;

use crate::platform::drivers::keyboard::KeyboardDriver;
use alloc::ffi::CString;
use alloc::string::ToString;
use core::str::FromStr;

#[panic_handler]
#[cfg(not(test))]
fn panic(info: &::core::panic::PanicInfo) -> ! {
    unsafe {
        SerialDriver::write(b"Panic occurred, attempting to print message:\n");
        let message = info.to_string();
        SerialDriver::println(&message);
        hcf()
    }
}

unsafe extern "C" {

    /// Halt and catch fire; implemented in C
    fn hcf() -> !;
}

use crate::platform::drivers::serial::SerialDriver;
use crate::platform::elf::Elf;
use crate::platform::modules::Modules;
use crate::platform::scheduler::Scheduler;
use crate::platform::ticker::Ticker;
use crate::platform::timer::Timer;
use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;

fn main() {
    unsafe {
        SerialDriver::println("Hello from Rust!");

        Timer::init(100);

        KeyboardDriver::init();

        Ticker::init();

        let init_elf_string = CString::from_str("init.elf").expect("Failed to create CString");
        let init_elf = Modules::find(init_elf_string.as_c_str()).unwrap();
        let mut init_ctx = VirtualMemoryManagerContext::create();
        let entrypoint_vaddr = Elf::load(&mut init_ctx, init_elf).unwrap();
        Scheduler::create_user(&mut init_ctx, entrypoint_vaddr);

        Scheduler::start();
    }
}
