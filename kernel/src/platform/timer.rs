mod bindings {
    include_bindings!("timer.rs");
}

pub struct Timer;

impl Timer {
    pub unsafe fn init(freq_hz: u32) {
        unsafe {
            bindings::timer_init(freq_hz);
        }
    }
}
