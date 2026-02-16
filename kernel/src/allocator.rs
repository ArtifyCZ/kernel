use crate::platform::drivers::serial::SerialDriver;
use crate::platform::memory_layout::PAGE_FRAME_SIZE;
use crate::platform::physical_memory_manager::PhysicalMemoryManager;
use crate::platform::virtual_address::VirtualAddress;
use crate::platform::virtual_address_allocator::VirtualAddressAllocator;
use crate::platform::virtual_memory_manager_context::{
    VirtualMemoryManagerContext, VirtualMemoryMappingFlags,
};
use crate::platform::virtual_page_address::VirtualPageAddress;
use crate::spin_lock::SpinLock;
use core::alloc::{GlobalAlloc, Layout};
use core::ops::Add;
use core::ptr::null_mut;

static mut NEXT_AVAILABLE_VIRTUAL_ADDRESS: Option<VirtualAddress> = None;

static mut LAST_MAPPED_VIRTUAL_PAGE: Option<VirtualPageAddress> = None;

static mut VMM_CONTEXT: Option<VirtualMemoryManagerContext> = None;

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

#[allow(static_mut_refs)]
unsafe impl GlobalAlloc for Allocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let layout = layout.pad_to_align();
        let _ = HEAP_LOCK.lock();
        let vmm_context = unsafe {
            if let Some(ref mut vmm_context) = VMM_CONTEXT {
                vmm_context
            } else {
                let vmm_context = VirtualMemoryManagerContext::get_kernel_context();
                VMM_CONTEXT = Some(vmm_context);
                VMM_CONTEXT.as_mut().unwrap_unchecked()
            }
        };

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
                    VirtualAddressAllocator::alloc_range(PAGE_FRAME_SIZE * 1024 * 1024)
                }
            };

            if unsafe { vmm_context.translate(next_virt_page_addr) }
                .unwrap()
                .is_some()
            {
                unsafe { SerialDriver::write(b"Page already mapped\n") };
                return null_mut();
            }

            if unsafe {
                vmm_context.map_page(
                    next_virt_page_addr,
                    new_page,
                    VirtualMemoryMappingFlags::PRESENT | VirtualMemoryMappingFlags::WRITE,
                )
            }
            .is_err()
            {
                unsafe { SerialDriver::write(b"Failed to map page\n") };
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
                SerialDriver::write(b"Allocated memory successfully!\n");
                ptr
            } else {
                null_mut()
            }
        }
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // Do nothing for now
        // @TODO: implement deallocation as well
    }
}
