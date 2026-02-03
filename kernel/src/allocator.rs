use core::alloc::{GlobalAlloc, Layout};
use crate::serial_println;
use core::ffi::c_char;
use core::ptr::null_mut;
use crate::platform::memory_layout::{KERNEL_HEAP_BASE, KERNEL_HEAP_MAX, PAGE_FRAME_SIZE};
use crate::platform::physical_memory_manager::pmm_alloc_frame;
use crate::platform::virtual_memory_manager::{vmm_map_page, vmm_translate};

static mut NEXT_FREE_BYTE: usize = 0;

static mut MAPPED_END: usize = 0;

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
