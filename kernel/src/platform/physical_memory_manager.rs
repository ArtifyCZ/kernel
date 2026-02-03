unsafe extern "C" {
    // uintptr_t pmm_alloc_frame(void);
    pub fn pmm_alloc_frame() -> usize;
}
