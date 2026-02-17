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
use core::ffi::c_void;
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
use crate::platform::syscalls::Syscalls;
use crate::platform::terminal::Terminal;
use crate::platform::ticker::Ticker;
use crate::platform::timer::Timer;
use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;

unsafe extern "C" fn thread_heartbeat(_args: *mut c_void) {
    let mut i = 0;
    loop {
        if i == 2000000 {
            unsafe {
                Terminal::print_char('.');
            }
            i = 0;
        }
        i += 1;
    }
}

unsafe extern "C" fn thread_keyboard(_args: *mut c_void) {
    loop {
        unsafe {
            if let Some(c) = KeyboardDriver::get_char() {
                Terminal::print_char(c);
            }
        }
    }
}

fn main(hhdm_offset: u64) {
    unsafe {
        SerialDriver::println("Hello from Rust!");

        Syscalls::init();
        Elf::init(hhdm_offset);
        Timer::init(100);

        KeyboardDriver::init();

        Ticker::init();

        Scheduler::init();

        Scheduler::create_kernel(thread_heartbeat);
        Scheduler::create_kernel(thread_keyboard);

        {
            let init_elf_string = CString::from_str("init.elf").expect("Failed to create CString");
            let init_elf = Modules::find(init_elf_string.as_c_str()).unwrap();
            let mut init_ctx = VirtualMemoryManagerContext::create();
            let entrypoint_vaddr = Elf::load(&mut init_ctx, init_elf).unwrap();
            Scheduler::create_user(&mut init_ctx, entrypoint_vaddr);
        }

        Scheduler::start();
    }
}
