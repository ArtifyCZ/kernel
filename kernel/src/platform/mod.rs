pub mod memory_layout;
pub mod physical_memory_manager;
pub mod physical_page_frame;
pub mod virtual_address;
pub mod virtual_memory_manager;
pub mod virtual_page_address;

mod bindings {
    include!(concat!(env!("OUT_DIR"), "/platform_bindings.rs"));
}

pub use bindings::serial_println;
