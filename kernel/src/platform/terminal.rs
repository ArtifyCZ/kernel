use core::ffi::c_char;

mod bindings {
    include_bindings!("terminal.rs");
}

pub struct Terminal;

impl Terminal {
    pub unsafe fn print_char(c: char) {
        let c: c_char = c as c_char;
        unsafe {
            bindings::terminal_print_char(c);
        }
    }

    pub unsafe fn print(message: &str) {
        for c in message.chars() {
            unsafe {
                Self::print_char(c);
            }
        }
    }
}
