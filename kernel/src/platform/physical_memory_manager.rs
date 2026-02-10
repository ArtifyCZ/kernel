use thiserror_no_std::Error;
use crate::platform::bindings;
use crate::platform::physical_page_frame::{PhysicalPageFrame, PhysicalPageFrameParseError};

pub struct PhysicalMemoryManager;

#[derive(Debug, Error)]
pub enum PhysicalMemoryManagerAllocFrameError {
    #[error("No available physical memory pages")]
    NoAvailablePages,
    #[error(transparent)]
    InvalidPageFrame(#[from] PhysicalPageFrameParseError),
}

impl PhysicalMemoryManager {
    pub unsafe fn alloc_frame()
    -> Result<PhysicalPageFrame, PhysicalMemoryManagerAllocFrameError> {
        let page_frame = unsafe { bindings::pmm_alloc_frame() };
        if page_frame == 0 {
            return Err(PhysicalMemoryManagerAllocFrameError::NoAvailablePages);
        }

        Ok(PhysicalPageFrame::new(page_frame)?)
    }
}
