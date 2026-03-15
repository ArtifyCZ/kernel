use core::ffi::c_void;
use kernel_bindings_gen::{limine_framebuffer, limine_module_response};

pub struct Platform;

impl Platform {
    pub unsafe fn init(
        framebuffer: *mut limine_framebuffer,
        modules: *mut limine_module_response,
        rsdp_address: u64,
    ) {
        unsafe {
            let config = kernel_bindings_gen::platform_config {
                framebuffer,
                modules,
                rsdp_address: rsdp_address as *mut c_void,
            };
            kernel_bindings_gen::platform_init(&config);
        }
    }
}
