use core::ffi::c_void;

pub(super) mod bindings {
    include_bindings!("timer.rs");
}

pub struct Timer;

impl Timer {
    pub unsafe fn init(freq_hz: u32) {
        unsafe {
            bindings::timer_init(freq_hz);
        }
    }

    pub(super) unsafe fn set_tick_handler(tick_handler: bindings::timer_tick_handler_t, arg: *mut c_void) {
        unsafe {
            bindings::timer_set_tick_handler(tick_handler, arg);
        }
    }

    pub unsafe fn get_ticks() -> u64 {
        unsafe {
            bindings::timer_get_ticks()
        }
    }
}
