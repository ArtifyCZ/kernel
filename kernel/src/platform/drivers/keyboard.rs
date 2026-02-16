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
}
