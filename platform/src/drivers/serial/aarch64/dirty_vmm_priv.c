#include "dirty_vmm_priv.h"

#include <stdint.h>

#include "physical_memory_manager.h"

// AArch64 Descriptor Bits
#define PTE_VALID      (1ULL << 0)
#define PTE_TABLE      (1ULL << 1)
#define PTE_PAGE       (1ULL << 1)
#define PTE_AF         (1ULL << 10)  // Access Flag
#define PTE_SH_INNER   (3ULL << 8)   // Inner Shareable
#define PTE_MEMATTR_DEV 0            // Index 0 in MAIR (we will set this below)
#define PTE_RW         (1ULL << 6)   // Read/Write

// @FIXME: this whole file is a *really* dirty way to get somehow serial working
extern uint64_t g_hhdm_offset;

void init_mair(void) {
    // Attr 0: 00000000 (Device-nGnRnE)
    // Attr 1: 11111111 (Normal, WB/WA/RA)
    uint64_t mair = 0xFF00;
    __asm__ volatile("msr mair_el1, %0" : : "r"(mair));
}

static uint64_t* get_vaddr(uint64_t paddr) {
    return (uint64_t*)(paddr + g_hhdm_offset);
}

void dirty_bootstrap_map_uart(uintptr_t vaddr, uintptr_t paddr) {
    uint64_t ttbr1;
    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(ttbr1));
    uint64_t* table = get_vaddr(ttbr1 & ~0xFFFULL);

    // Walk L0 -> L1 -> L2 -> L3
    for (int level = 0; level < 3; level++) {
        int shift = 39 - (level * 9);
        int idx = (vaddr >> shift) & 0x1FF;

        if (!(table[idx] & PTE_VALID)) {
            // Allocate a new table page if missing
            uintptr_t new_table = pmm_alloc_frame();
            // Clear the new page!
            for(int i=0; i<512; i++) ((uint64_t*)get_vaddr(new_table))[i] = 0;

            table[idx] = new_table | PTE_TABLE | PTE_VALID;
        }
        table = get_vaddr(table[idx] & ~0xFFFULL);
    }

    // Level 3 Entry
    int idx = (vaddr >> 12) & 0x1FF;
    table[idx] = (paddr & ~0xFFFULL) | PTE_PAGE | PTE_VALID | PTE_AF |
                 (PTE_MEMATTR_DEV << 2) | PTE_RW | PTE_SH_INNER;

    // Flush TLB (Crucial!)
    __asm__ volatile("tlbi vmalle1is; dsb sy; isb");
}
