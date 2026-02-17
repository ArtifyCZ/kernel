use crate::platform::drivers::serial::SerialDriver;
use crate::platform::memory_layout::PAGE_FRAME_SIZE;
use crate::platform::scheduler::bindings::{interrupts_disable, interrupts_enable};
use crate::platform::thread::bindings::thread_ctx;
use crate::platform::thread::{thread_prepare_switch, thread_setup_kernel, thread_setup_user};
use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;
use alloc::boxed::Box;
use core::ffi::c_void;
use core::pin::Pin;
use core::ptr::null_mut;

mod bindings {
    include_bindings!("scheduler.rs");
}

#[derive(Eq, PartialEq, Copy, Clone)]
#[repr(C)]
enum ThreadState {
    Unused,
    Running,
    Runnable,
    Dead,
}

impl Default for ThreadState {
    fn default() -> Self {
        ThreadState::Unused
    }
}

#[repr(C, align(16))]
struct Thread {
    state: ThreadState,
    stack: Pin<Box<[u8; 4 * PAGE_FRAME_SIZE]>>,
    ctx: *mut super::thread::bindings::thread_ctx,
}

impl Default for Thread {
    fn default() -> Self {
        Self {
            state: Default::default(),
            stack: Box::into_pin(unsafe { Box::new_zeroed().assume_init() }),
            ctx: null_mut(),
        }
    }
}

unsafe impl Send for Thread {}
unsafe impl Sync for Thread {}

#[repr(C)]
pub struct Scheduler {
    current_thread: i32,
    started: bool,
    threads: [Thread; 12],
}

impl Default for Scheduler {
    fn default() -> Self {
        Scheduler {
            current_thread: -1,
            started: false,
            threads: [(); 12].map(|_| Default::default()),
        }
    }
}

static mut SCHEDULER: Option<Box<Scheduler>> = None;

impl Scheduler {
    pub unsafe fn init() {
        unsafe {
            SerialDriver::println("Initializing scheduler...");
            SCHEDULER = Some(Default::default());
            SerialDriver::println("Scheduler initialized!");
        }
    }

    #[allow(static_mut_refs)]
    pub unsafe fn start() -> ! {
        loop {
            unsafe {
                let scheduler = SCHEDULER.as_mut().unwrap().as_mut();
                scheduler.started = true;
                bindings::sched_start();
            }
        }
    }

    fn find_first_unused_thread(&mut self) -> Option<(usize, &mut Thread)> {
        self.threads
            .iter_mut()
            .enumerate()
            .filter(|(_, thread)| thread.state == ThreadState::Unused)
            .next()
    }

    fn find_next_runnable_thread(&mut self) -> Option<(usize, &mut Thread)> {
        for offset in 1..=self.threads.len() {
            let idx = if self.current_thread < 0 {
                offset - 1
            } else {
                ((self.current_thread as usize) + offset) % self.threads.len()
            };
            if self.threads[idx].state == ThreadState::Runnable {
                return Some((idx, &mut self.threads[idx]));
            }
        }

        None
    }

    #[allow(static_mut_refs)]
    pub unsafe fn create_user(
        user_ctx: &mut VirtualMemoryManagerContext,
        entrypoint_vaddr: usize,
    ) -> i32 {
        unsafe {
            bindings::interrupts_disable(); // @TODO: use separate interrupts bindings
            let scheduler = SCHEDULER.as_mut().unwrap().as_mut();
            let (thread_idx, thread) = scheduler
                .find_first_unused_thread()
                .expect("No unused thread available");
            let kernel_stack_top = thread.stack.as_ptr() as usize + thread.stack.len();
            let thread_ctx = thread_setup_user(user_ctx, entrypoint_vaddr, kernel_stack_top);
            thread.ctx = thread_ctx;
            thread.state = ThreadState::Runnable;
            bindings::interrupts_enable(); // @TODO: use separate interrupts bindings
            thread_idx as i32
        }
    }

    #[allow(static_mut_refs)]
    pub unsafe fn create_kernel<F>(function: F) -> i32
    where
        F: FnOnce() + 'static,
    {
        unsafe extern "C" fn trampoline<F>(args: *mut c_void) where F: FnOnce() + 'static {
            let f = unsafe { Box::from_raw(args as *mut F) };
            f();
            todo!("Invoking syscalls from Rust not implemented yet (should use sys_exit)")
        }

        let args = Box::into_raw(Box::new(function)).cast();
        let function = trampoline::<F>;

        unsafe {
            bindings::interrupts_disable(); // @TODO: use separate interrupts bindings
            let scheduler = SCHEDULER.as_mut().unwrap().as_mut();
            let (thread_idx, thread) = scheduler
                .find_first_unused_thread()
                .expect("No unused thread available");
            let stack_top = thread.stack.as_ptr() as usize + thread.stack.len();
            let thread_ctx =thread_setup_kernel(stack_top, function, args);
            thread.ctx = thread_ctx;
            thread.state = ThreadState::Runnable;
            bindings::interrupts_enable(); // @TODO: use separate interrupts bindings
            thread_idx as i32
        }
    }

    #[allow(static_mut_refs)]
    pub unsafe fn thread_exit(prev_thread_ctx: *mut thread_ctx) -> *mut thread_ctx {
        unsafe {
            interrupts_disable();
            let scheduler = SCHEDULER.as_mut().unwrap().as_mut();
            let prev_idx = scheduler.current_thread;
            scheduler.threads[prev_idx as usize].state = ThreadState::Dead;
            sched_heartbeat(prev_thread_ctx)
        }
    }
}

#[allow(static_mut_refs)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sched_heartbeat(prev_thread_ctx: *mut thread_ctx) -> *mut thread_ctx {
    unsafe {
        interrupts_disable();
        let scheduler = match SCHEDULER.as_mut() {
            None => {
                interrupts_enable();
                return prev_thread_ctx;
            }
            Some(scheduler) => scheduler.as_mut(),
        };
        if !scheduler.started {
            interrupts_enable();
            return prev_thread_ctx;
        }

        let prev_idx = scheduler.current_thread;

        let (next_idx, next_thread) = match scheduler.find_next_runnable_thread() {
            None => {
                interrupts_enable();
                return prev_thread_ctx;
            }
            Some((next_idx, next_thread)) => (next_idx as i32, next_thread),
        };

        if next_idx == prev_idx {
            interrupts_enable();
            return prev_thread_ctx;
        }

        next_thread.state = ThreadState::Running;
        let kernel_stack_top = next_thread.stack.as_ptr() as usize + next_thread.stack.len();
        thread_prepare_switch(kernel_stack_top);
        let next_thread_ctx = next_thread.ctx;

        if prev_idx >= 0 {
            let prev_thread = &mut scheduler.threads[prev_idx as usize];
            if prev_thread.state == ThreadState::Running {
                prev_thread.state = ThreadState::Runnable;
            }
            prev_thread.ctx = prev_thread_ctx;
        }

        scheduler.current_thread = next_idx;

        interrupts_enable();
        next_thread_ctx
    }
}
