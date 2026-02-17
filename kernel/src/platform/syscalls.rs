use crate::platform::drivers::serial::SerialDriver;

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
