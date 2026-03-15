use core::ffi::c_void;
use kernel_bindings_gen::{limine_framebuffer, platform_config, platform_init};

pub struct Platform;

impl Platform {
    pub unsafe fn init(framebuffer: *mut limine_framebuffer, rsdp_address: u64) {
        unsafe {
            let config = platform_config {
                framebuffer,
                rsdp_address: rsdp_address as *mut c_void,
            };
            platform_init(&config);
        }
    }
}
