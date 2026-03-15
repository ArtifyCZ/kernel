use kernel_bindings_gen::{early_console_disable, early_console_init, early_console_write};

pub struct EarlyConsole;

impl EarlyConsole {
    pub unsafe fn init(serial_base: usize) {
        unsafe {
            early_console_init(serial_base);
        }
    }

    pub unsafe fn disable() {
        unsafe {
            early_console_disable();
        }
    }

    pub unsafe fn write(byte: u8) {
        unsafe {
            early_console_write(byte);
        }
    }

    pub unsafe fn write_str(str: &str) {
        for byte in str.bytes() {
            unsafe {
                EarlyConsole::write(byte);
            }
        }
    }
}
