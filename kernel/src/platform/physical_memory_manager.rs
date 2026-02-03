use crate::platform::physical_page_frame::{PhysicalPageFrame, PhysicalPageFrameParseError};

unsafe extern "C" {
    // uintptr_t pmm_alloc_frame(void);
    fn pmm_alloc_frame() -> usize;
}

pub struct PhysicalMemoryManager;

#[derive(Debug)]
pub enum PhysicalMemoryManagerAllocFrameError {
    NoAvailablePages,
    InvalidPageFrame(PhysicalPageFrameParseError),
}

impl PhysicalMemoryManager {
    pub unsafe fn alloc_frame()
    -> Result<PhysicalPageFrame, PhysicalMemoryManagerAllocFrameError> {
        let page_frame = unsafe { pmm_alloc_frame() };
        if page_frame == 0 {
            return Err(PhysicalMemoryManagerAllocFrameError::NoAvailablePages);
        }

        Ok(PhysicalPageFrame::new(page_frame)?)
    }
}

impl From<PhysicalPageFrameParseError> for PhysicalMemoryManagerAllocFrameError {
    fn from(value: PhysicalPageFrameParseError) -> Self {
        PhysicalMemoryManagerAllocFrameError::InvalidPageFrame(value)
    }
}
