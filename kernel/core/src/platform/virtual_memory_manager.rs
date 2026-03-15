pub struct VirtualMemoryManager;

impl VirtualMemoryManager {
    pub unsafe fn init(hhdm_offset: u64) {
        unsafe {
            kernel_bindings_gen::vmm_init(hhdm_offset);
        }
    }
}
