use crate::platform::virtual_memory_manager_context::VirtualMemoryManagerContext;
use core::ffi::c_void;
use crate::platform::thread::bindings::thread_ctx;

pub(super) mod bindings {
    include_bindings!("thread.rs");
}

pub(super) unsafe fn thread_prepare_switch(kernel_stack_top: usize) {
    unsafe {
        bindings::thread_prepare_switch(kernel_stack_top);
    }
}

pub(super) unsafe fn thread_setup_user(
    user_ctx: &mut VirtualMemoryManagerContext,
    entrypoint_vaddr: usize,
    kernel_stack_top: usize,
) -> *mut thread_ctx {
    unsafe {
        let user_ctx = core::mem::transmute(user_ctx.inner_mut());
        bindings::thread_setup_user(user_ctx, entrypoint_vaddr, kernel_stack_top)
    }
}

pub(super) unsafe fn thread_setup_kernel(
    stack_top: usize,
    function: unsafe extern "C" fn(arg: *mut c_void) -> (),
    arg: *mut c_void,
) -> *mut thread_ctx {
    unsafe {
        bindings::thread_setup_kernel(stack_top, Some(function), arg)
    }
}
