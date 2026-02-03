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
    pub fn vmm_map_page(virt_addr: usize, phys_addr: usize, flags: usize) -> bool;

    // uint64_t vmm_translate(uint64_t virt_addr);
    // Returns 0 if not mapped
    pub fn vmm_translate(virt_addr: usize) -> usize;
}
