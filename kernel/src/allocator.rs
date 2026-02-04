use crate::platform::memory_layout::KERNEL_HEAP_BASE;
use crate::platform::physical_memory_manager::PhysicalMemoryManager;
use crate::platform::virtual_address::VirtualAddress;
use crate::platform::virtual_memory_manager::VirtualMemoryManager;
use crate::platform::virtual_page_address::VirtualPageAddress;
use crate::serial_println;
use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_char;
use core::ops::Add;
use core::ptr::null_mut;
use crate::spin_lock::SpinLock;

static mut NEXT_AVAILABLE_VIRTUAL_ADDRESS: Option<VirtualAddress> = None;

static mut LAST_MAPPED_VIRTUAL_PAGE: Option<VirtualPageAddress> = None;

static HEAP_LOCK: SpinLock = SpinLock::new();

pub struct Allocator;

#[global_allocator]
pub static GLOBAL_ALLOCATOR: Allocator = Allocator;

impl Add<usize> for VirtualAddress {
    type Output = VirtualAddress;

    fn add(self, rhs: usize) -> Self::Output {
        VirtualAddress::new(self.inner() + rhs).unwrap()
    }
}

unsafe impl GlobalAlloc for Allocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        HEAP_LOCK.lock();
        loop {
            let next_available_virtual_address = unsafe { NEXT_AVAILABLE_VIRTUAL_ADDRESS };
            let last_mapped_virtual_page = unsafe { LAST_MAPPED_VIRTUAL_PAGE };
            if let Some(last_mapped_virtual_page) = last_mapped_virtual_page {
                if let Some(next_available_virtual_address) = next_available_virtual_address {
                    if next_available_virtual_address + layout.size()
                        <= last_mapped_virtual_page.end()
                    {
                        break;
                    }
                }
            }


            let new_page = unsafe {
                PhysicalMemoryManager::alloc_frame()
                    .expect("Failed to allocate physical page frame")
            };

            let next_virt_page_addr = unsafe {
                if let Some(last_mapped_virtual_page) = LAST_MAPPED_VIRTUAL_PAGE {
                    last_mapped_virtual_page.next_page()
                } else {
                    VirtualPageAddress::new(KERNEL_HEAP_BASE).unwrap()
                }
            };

            if unsafe { VirtualMemoryManager::translate(next_virt_page_addr) }
                .unwrap()
                .is_some()
            {
                unsafe { serial_println(b"Page already mapped\n\0".as_ptr() as *const c_char) };
                return null_mut();
            }

            if unsafe { VirtualMemoryManager::map_page(next_virt_page_addr, new_page) }.is_err() {
                unsafe { serial_println(b"Failed to map page\n\0".as_ptr() as *const c_char) };
                return null_mut();
            }

            unsafe {
                LAST_MAPPED_VIRTUAL_PAGE = Some(next_virt_page_addr);

                if let None = NEXT_AVAILABLE_VIRTUAL_ADDRESS {
                    NEXT_AVAILABLE_VIRTUAL_ADDRESS = Some(next_virt_page_addr.start())
                }
            }
        }

        unsafe {
            if let Some(next_available_virtual_address) = NEXT_AVAILABLE_VIRTUAL_ADDRESS {
                let ptr = next_available_virtual_address.inner() as *mut u8;
                NEXT_AVAILABLE_VIRTUAL_ADDRESS =
                    Some(next_available_virtual_address + layout.size());
                serial_println(b"Allocated memory successfully!\n\0".as_ptr() as *const c_char);
                ptr
            } else {
                null_mut()
            }
        }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        // Do nothing for now
        // @TODO: implement deallocation as well
    }
}
