use core::ffi::c_char;

mod bindings {
    include_bindings!("drivers/keyboard.rs");
}

pub struct KeyboardDriver;

impl KeyboardDriver {
    pub unsafe fn init() {
        unsafe {
            bindings::keyboard_init();
        }
    }

    pub unsafe fn get_char() -> Option<char> {
        unsafe {
            let mut c: c_char = b'\0' as c_char;
            if bindings::keyboard_get_char(&raw mut c) {
                Some((c as u8) as char)
            } else {
                None
            }
        }
    }
}
