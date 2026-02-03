use crate::platform::memory_layout::{KERNEL_HEAP_BASE, KERNEL_HEAP_MAX, PAGE_FRAME_SIZE};
use crate::platform::physical_memory_manager::PhysicalMemoryManager;
use crate::platform::virtual_memory_manager::{vmm_map_page, vmm_translate};
use crate::serial_println;
use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_char;
use core::ptr::null_mut;

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
            PhysicalMemoryManager::alloc_frame().expect("Failed to allocate physical page frame")
        };

        unsafe {
            match map_page(new_page.inner()) {
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
