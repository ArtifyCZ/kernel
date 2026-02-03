use core::alloc::{GlobalAlloc, Layout};
use crate::serial_println;
use core::ffi::c_char;
use core::ptr::null_mut;

pub const PAGE_FRAME_SIZE: usize = 0x1000; // 4 KiB

pub const KERNEL_HEAP_SIZE: usize = 256 * 1024 * 1024; // 256 MiB
pub const KERNEL_HEAP_BASE: usize = 0xFFFF_C000_0000_0000;
pub const KERNEL_HEAP_MAX: usize = KERNEL_HEAP_BASE + KERNEL_HEAP_SIZE;

static mut NEXT_FREE_BYTE: usize = 0;

static mut MAPPED_END: usize = 0;

unsafe extern "C" {
    // uintptr_t pmm_alloc_frame(void);
    fn pmm_alloc_frame() -> usize;

    // Page table flags (x86_64)
    // #define VMM_PTE_P   (1ull << 0)   // Present
    // #define VMM_PTE_W   (1ull << 1)   // Writable
    // #define VMM_PTE_U   (1ull << 2)   // User
    // #define VMM_PTE_PWT (1ull << 3)
    // #define VMM_PTE_PCD (1ull << 4)
    // #define VMM_PTE_A   (1ull << 5)
    // #define VMM_PTE_D   (1ull << 6)
    // #define VMM_PTE_PS  (1ull << 7)   // Large page (not used for 4KiB mappings)
    // #define VMM_PTE_G   (1ull << 8)
    // #define VMM_PTE_NX  (1ull << 63)  // No-execute
    // bool vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
    fn vmm_map_page(virt_addr: usize, phys_addr: usize, flags: usize) -> bool;

    // uint64_t vmm_translate(uint64_t virt_addr);
    // Returns 0 if not mapped
    fn vmm_translate(virt_addr: usize) -> usize;
}

struct Allocator;

unsafe impl GlobalAlloc for Allocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        unsafe { malloc(layout.size()) }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        // Do nothing for now
        // @TODO: implement deallocation as well
    }
}

#[global_allocator]
static GLOBAL_ALLOCATOR: Allocator = Allocator;

unsafe fn alloc_frame() -> Result<usize, ()> {
    let page_frame = unsafe { pmm_alloc_frame() };
    if page_frame == 0 {
        return Err(());
    }

    Ok(page_frame)
}

unsafe fn map_page(page_frame: usize) -> Result<(), ()> {
    unsafe {
        if NEXT_FREE_BYTE == 0 {
            NEXT_FREE_BYTE = KERNEL_HEAP_BASE;
        }
    }

    let next_page_virt_addr = unsafe {
        if MAPPED_END == 0 {
            KERNEL_HEAP_BASE
        } else {
            MAPPED_END + 1 // increment and we are in the next page
        }
    };

    let new_mapped_end = unsafe {
        if MAPPED_END == 0 {
            KERNEL_HEAP_BASE + PAGE_FRAME_SIZE - 1 // decrement so that it does not overflow into the next page
        } else {
            MAPPED_END + PAGE_FRAME_SIZE
        }
    };

    if next_page_virt_addr > KERNEL_HEAP_MAX {
        unsafe { serial_println(b"Exceeded maximum address space\n\0".as_ptr() as *const c_char) };
        return Err(());
    }

    if next_page_virt_addr % PAGE_FRAME_SIZE != 0 {
        unsafe { serial_println(b"Page alignment error\n\0".as_ptr() as *const c_char) };
        return Err(());
    }

    if unsafe { vmm_translate(next_page_virt_addr) } != 0 {
        unsafe { serial_println(b"Page already mapped\n\0".as_ptr() as *const c_char) };
        return Err(());
    }

    if unsafe { vmm_map_page(next_page_virt_addr, page_frame, 0x02) } == false {
        unsafe { serial_println(b"Failed to map page\n\0".as_ptr() as *const c_char) };
        return Err(());
    }

    unsafe {
        MAPPED_END = new_mapped_end;
    }

    Ok(())
}

pub unsafe fn malloc(size: usize) -> *mut u8 {
    loop {
        if unsafe { NEXT_FREE_BYTE } + size <= unsafe { MAPPED_END } {
            break;
        }

        let new_page = unsafe {
            match alloc_frame() {
                Ok(page) => page,
                Err(()) => {
                    serial_println(b"Failed to allocate memory\n\0".as_ptr() as *const c_char);
                    return null_mut();
                }
            }
        };
        if new_page == 0 {
            return null_mut(); // Failed to alloc a new frame
        }

        unsafe {
            match map_page(new_page) {
                Ok(()) => (),
                Err(()) => return null_mut(),
            }
        }
    }

    unsafe {
        let ptr = NEXT_FREE_BYTE as *mut u8;
        NEXT_FREE_BYTE += size;

        ptr
    }
}
