use alloc::format;
use crate::platform::timer::Timer;
use core::ffi::c_void;
use core::ptr::null_mut;
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::scheduler::Scheduler;
use crate::platform::tasks::TaskState;

pub struct Ticker;

impl Ticker {
    pub unsafe fn init() {
        unsafe {
            Timer::set_tick_handler(Some(Self::tick_handler), null_mut());
        }
    }

    unsafe extern "C" fn tick_handler(
        frame: *mut *mut super::timer::bindings::interrupt_frame,
        _arg: *mut c_void,
    ) -> bool {
        unsafe {
            let prev_frame: *mut super::timer::bindings::interrupt_frame = frame.read();
            let prev_state = TaskState(prev_frame.cast());
            let mut scheduler = Scheduler::get_instance();
            let next_state: TaskState = scheduler.heartbeat(prev_state);
            let next_frame: *mut super::timer::bindings::interrupt_frame = next_state.0.cast();
            frame.write(next_frame);

            let ticks = Timer::get_ticks();
            if ticks % 100 == 0 {
                SerialDriver::println(&format!("Timer ticks: {}", ticks));
            }

            true
        }
    }
}
