pub const PAGE_FRAME_SIZE: usize = 0x1000; // 4 KiB

pub const KERNEL_HEAP_SIZE: usize = 256 * 1024 * 1024; // 256 MiB
pub const KERNEL_HEAP_BASE: usize = 0xFFFF_C000_0000_0000;
pub const KERNEL_HEAP_MAX: usize = KERNEL_HEAP_BASE + KERNEL_HEAP_SIZE;
