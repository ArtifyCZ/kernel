use crate::platform::physical_page_frame::{PhysicalPageFrame, PhysicalPageFrameParseError};
use crate::platform::virtual_page_address::VirtualPageAddress;

mod bindings {
    include_bindings!("virtual_memory_manager.rs");
}

pub(super) const VMM_PAGE_SIZE: usize = bindings::VMM_PAGE_SIZE as usize;

pub struct VirtualMemoryManager;

impl VirtualMemoryManager {
    /// @TODO: add support for the flags
    /// @TODO: add better errors
    pub unsafe fn map_page(
        virtual_page_address: VirtualPageAddress,
        physical_address: PhysicalPageFrame,
    ) -> Result<(), ()> {
        if unsafe {
            bindings::vmm_map_page(
                &raw mut bindings::g_kernel_context,
                virtual_page_address.inner(),
                physical_address.inner(),
                bindings::vmm_flags_t_VMM_FLAG_PRESENT | bindings::vmm_flags_t_VMM_FLAG_WRITE,
            )
        } {
            Ok(())
        } else {
            Err(())
        }
    }

    pub unsafe fn translate(
        virtual_page_address: VirtualPageAddress,
    ) -> Result<Option<PhysicalPageFrame>, PhysicalPageFrameParseError> {
        let physical_page_frame = unsafe {
            bindings::vmm_translate(
                &raw mut bindings::g_kernel_context,
                virtual_page_address.inner(),
            )
        };
        if physical_page_frame == 0 {
            return Ok(None);
        }

        Ok(Some(PhysicalPageFrame::new(physical_page_frame)?))
    }
}
