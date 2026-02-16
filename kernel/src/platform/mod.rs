macro_rules! include_bindings {
    ($module:tt) => {
        include!(concat!(env!("OUT_DIR"), "/bindings/", $module));
    };
}

pub mod memory_layout;
pub mod physical_memory_manager;
pub mod physical_page_frame;
pub mod scheduler;
pub mod virtual_address;
pub mod virtual_address_allocator;
pub mod virtual_memory_manager_context;
pub mod virtual_page_address;

mod serial_bindings {
    include_bindings!("drivers/serial.rs");
}

pub use serial_bindings::serial_println;
