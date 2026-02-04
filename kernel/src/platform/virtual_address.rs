use thiserror_no_std::Error;
use crate::platform::memory_layout::{KERNEL_HEAP_BASE, KERNEL_HEAP_MAX, PAGE_FRAME_SIZE};

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq)]
pub struct VirtualAddress(usize);

#[derive(Debug, Error)]
pub enum VirtualAddressParseError {
    #[error("Virtual address is not in the kernel's address space: range {0:#x} - {1:#x}")]
    NotInRange(usize, usize),
}

impl VirtualAddress {
    pub const fn new(value: usize) -> Result<Self, VirtualAddressParseError> {
        if value < KERNEL_HEAP_BASE || value > KERNEL_HEAP_MAX {
            return Err(VirtualAddressParseError::NotInRange(KERNEL_HEAP_BASE, KERNEL_HEAP_MAX));
        }

        Ok(VirtualAddress(value))
    }

    pub const fn inner(&self) -> usize {
        self.0
    }
}

impl TryFrom<usize> for VirtualAddress {
    type Error = VirtualAddressParseError;

    fn try_from(value: usize) -> Result<Self, Self::Error> {
        Self::new(value)
    }
}

impl Into<usize> for VirtualAddress {
    fn into(self) -> usize {
        self.0
    }
}
