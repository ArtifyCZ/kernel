use crate::platform::tasks::TaskFrame;
use alloc::boxed::Box;
use core::ffi::c_void;
use kernel_bindings_gen::{interrupt_frame, timer_get_ticks, timer_init, timer_set_tick_handler};

pub struct Timer;

impl Timer {
    pub unsafe fn init(freq_hz: u32) {
        unsafe {
            timer_init(freq_hz);
        }
    }

    pub fn set_tick_handler<F>(f: F)
    where
        F: (FnMut(TaskFrame) -> TaskFrame) + 'static,
    {
        unsafe extern "C" fn trampoline<F>(
            frame: *mut *mut interrupt_frame,
            arg: *mut c_void,
        ) -> bool
        where
            F: (FnMut(TaskFrame) -> TaskFrame) + 'static,
        {
            let f: *mut F = arg.cast();
            let f: &mut F = unsafe { &mut *f };
            let prev_frame = unsafe { frame.read() };
            let prev_frame: TaskFrame = TaskFrame(prev_frame.cast());
            let next_frame: TaskFrame = f(prev_frame);
            let next_frame: *mut interrupt_frame = next_frame.0.cast();
            unsafe {
                frame.write(next_frame);
            }
            true
        }

        unsafe {
            let f = Box::into_raw(Box::new(f));
            timer_set_tick_handler(Some(trampoline::<F>), f.cast());
        }
    }

    pub fn get_ticks() -> u64 {
        unsafe { timer_get_ticks() }
    }
}
