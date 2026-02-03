use crate::platform::physical_page_frame::{PhysicalPageFrame, PhysicalPageFrameParseError};
use crate::platform::virtual_page_address::VirtualPageAddress;

unsafe extern "C" {
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

pub struct VirtualMemoryManager;

impl VirtualMemoryManager {
    /// @TODO: add support for the flags
    /// @TODO: add better errors
    pub unsafe fn map_page(
        virtual_page_address: VirtualPageAddress,
        physical_address: PhysicalPageFrame,
    ) -> Result<(), ()> {
        if unsafe { vmm_map_page(virtual_page_address.inner(), physical_address.inner(), 0x02) } {
            Ok(())
        } else {
            Err(())
        }
    }

    pub unsafe fn translate(
        virtual_page_address: VirtualPageAddress,
    ) -> Result<Option<PhysicalPageFrame>, PhysicalPageFrameParseError> {
        let physical_page_frame = unsafe { vmm_translate(virtual_page_address.inner()) };
        if physical_page_frame == 0 {
            return Ok(None);
        }

        Ok(Some(PhysicalPageFrame::new(physical_page_frame)?))
    }
}
