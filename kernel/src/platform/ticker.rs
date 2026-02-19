use alloc::boxed::Box;
use alloc::format;
use crate::platform::timer::Timer;
use core::ffi::c_void;
use core::ptr::null_mut;
use crate::interrupt_safe_spin_lock::InterruptSafeSpinLock;
use crate::platform::drivers::serial::SerialDriver;
use crate::platform::scheduler::Scheduler;
use crate::platform::tasks::TaskState;

pub struct Ticker;

impl Ticker {
    pub unsafe fn init(scheduler: &'static InterruptSafeSpinLock<Scheduler>) {
        unsafe {
            Timer::set_tick_handler(Some(Self::tick_handler), scheduler as *const _ as *mut _);
        }
    }

    unsafe extern "C" fn tick_handler(
        frame: *mut *mut super::timer::bindings::interrupt_frame,
        scheduler: *mut c_void,
    ) -> bool {
        unsafe {
            let scheduler: &'static InterruptSafeSpinLock<Scheduler> = &*scheduler.cast();

            let prev_frame: *mut super::timer::bindings::interrupt_frame = frame.read();
            let prev_state = TaskState(prev_frame.cast());
            let next_state: TaskState = scheduler.lock().heartbeat(prev_state);
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
