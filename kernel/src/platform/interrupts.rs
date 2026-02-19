mod bindings {
    include_bindings!("interrupts.rs");
}

pub struct Interrupts;

impl Interrupts {
    pub unsafe fn init() {
        unsafe {
            bindings::interrupts_init();
        }
    }

    pub unsafe fn enable() {
        unsafe {
            bindings::interrupts_enable();
        }
    }

    pub unsafe fn disable() {
        unsafe {
            bindings::interrupts_disable();
        }
    }
}
