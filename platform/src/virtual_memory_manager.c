// platform/src/virtual_memory_manager.c
#include "virtual_memory_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "physical_memory_manager.h"
#include "serial.h"

#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ull

static uint64_t g_hhdm_offset = 0;

static inline void *phys_to_virt(uint64_t phys) {
    return (void *) (phys + g_hhdm_offset);
}

static inline uint64_t read_cr3(void) {
    uint64_t value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value) :: "memory");
    return value;
}

static inline void write_cr3(uint64_t value) {
    __asm__ volatile ("mov %0, %%cr3" :: "r"(value) : "memory");
}

static inline void invlpg(uint64_t virt) {
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

static inline bool is_aligned_4k(uint64_t x) {
    return (x & (VMM_PAGE_SIZE - 1)) == 0;
}

static inline uint16_t pml4_index(uint64_t va) { return (va >> 39) & 0x1FF; }
static inline uint16_t pdpt_index(uint64_t va) { return (va >> 30) & 0x1FF; }
static inline uint16_t pd_index(uint64_t va) { return (va >> 21) & 0x1FF; }
static inline uint16_t pt_index(uint64_t va) { return (va >> 12) & 0x1FF; }

static uint64_t *alloc_table_page(void) {
    void *phys = pmm_alloc_frame();
    if (phys == 0) {
        return NULL;
    }
    uint64_t *tbl = (uint64_t *) phys_to_virt((uint64_t) phys);
    // Zero it (page tables must start empty).
    for (size_t i = 0; i < 512; i++) {
        tbl[i] = 0;
    }
    return tbl;
}

// Returns pointer to next-level table, allocating it if missing.
// entry_flags are the flags to use for the intermediate page-table entries (usually P|W).
static uint64_t *get_or_create_next(uint64_t *table, uint16_t index, uint64_t entry_flags) {
    uint64_t e = table[index];

    if ((e & VMM_PTE_P) != 0) {
        // Present. If PS is set here, it's a large page mapping => not supported in this minimal VMM.
        if ((e & VMM_PTE_PS) != 0) {
            return NULL;
        }
        uint64_t next_phys = e & PTE_ADDR_MASK;
        return (uint64_t *) phys_to_virt(next_phys);
    }

    uint64_t *next = alloc_table_page();
    if (next == NULL) {
        return NULL;
    }

    uint64_t next_phys = (uint64_t) ((uintptr_t) next - g_hhdm_offset);
    table[index] = (next_phys & PTE_ADDR_MASK) | (entry_flags | VMM_PTE_P);

    return next;
}

void vmm_init(uint64_t hhdm_offset) {
    g_hhdm_offset = hhdm_offset;

    serial_print("VMM: HHDM offset = ");
    serial_print_hex_u64(g_hhdm_offset);
    serial_println("");

    uint64_t cr3 = read_cr3();
    serial_print("VMM: CR3 = ");
    serial_print_hex_u64(cr3);
    serial_println("");
}

bool vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    if (!is_aligned_4k(virt_addr) || !is_aligned_4k(phys_addr)) {
        return false;
    }

    uint64_t cr3 = read_cr3();
    uint64_t pml4_phys = cr3 & PTE_ADDR_MASK;
    uint64_t *pml4 = (uint64_t *) phys_to_virt(pml4_phys);

    // For intermediate levels: present + writable is typically fine for kernel page tables.
    const uint64_t mid_flags = VMM_PTE_P | VMM_PTE_W;

    uint64_t *pdpt = get_or_create_next(pml4, pml4_index(virt_addr), mid_flags);
    if (pdpt == NULL) return false;

    uint64_t *pd = get_or_create_next(pdpt, pdpt_index(virt_addr), mid_flags);
    if (pd == NULL) return false;

    uint64_t *pt = get_or_create_next(pd, pd_index(virt_addr), mid_flags);
    if (pt == NULL) return false;

    uint16_t pti = pt_index(virt_addr);

    // If already mapped, treat as failure for now (you can add a "remap" mode later).
    if ((pt[pti] & VMM_PTE_P) != 0) {
        return false;
    }

    pt[pti] = (phys_addr & PTE_ADDR_MASK) | (flags | VMM_PTE_P);

    invlpg(virt_addr);
    return true;
}

bool vmm_unmap_page(uint64_t virt_addr) {
    if (!is_aligned_4k(virt_addr)) {
        return false;
    }

    uint64_t cr3 = read_cr3();
    uint64_t pml4_phys = cr3 & PTE_ADDR_MASK;
    uint64_t *pml4 = (uint64_t *) phys_to_virt(pml4_phys);

    uint64_t e1 = pml4[pml4_index(virt_addr)];
    if ((e1 & VMM_PTE_P) == 0 || (e1 & VMM_PTE_PS) != 0) return false;
    uint64_t *pdpt = (uint64_t *) phys_to_virt(e1 & PTE_ADDR_MASK);

    uint64_t e2 = pdpt[pdpt_index(virt_addr)];
    if ((e2 & VMM_PTE_P) == 0 || (e2 & VMM_PTE_PS) != 0) return false;
    uint64_t *pd = (uint64_t *) phys_to_virt(e2 & PTE_ADDR_MASK);

    uint64_t e3 = pd[pd_index(virt_addr)];
    if ((e3 & VMM_PTE_P) == 0 || (e3 & VMM_PTE_PS) != 0) return false;
    uint64_t *pt = (uint64_t *) phys_to_virt(e3 & PTE_ADDR_MASK);

    uint16_t pti = pt_index(virt_addr);
    if ((pt[pti] & VMM_PTE_P) == 0) {
        return false;
    }

    pt[pti] = 0;
    invlpg(virt_addr);
    return true;
}

uint64_t vmm_translate(uint64_t virt_addr) {
    uint64_t cr3 = read_cr3();
    uint64_t pml4_phys = cr3 & PTE_ADDR_MASK;
    uint64_t *pml4 = (uint64_t *) phys_to_virt(pml4_phys);

    uint64_t e1 = pml4[pml4_index(virt_addr)];
    if ((e1 & VMM_PTE_P) == 0) return 0;
    if ((e1 & VMM_PTE_PS) != 0) return 0;

    uint64_t *pdpt = (uint64_t *) phys_to_virt(e1 & PTE_ADDR_MASK);
    uint64_t e2 = pdpt[pdpt_index(virt_addr)];
    if ((e2 & VMM_PTE_P) == 0) return 0;
    if ((e2 & VMM_PTE_PS) != 0) return 0;

    uint64_t *pd = (uint64_t *) phys_to_virt(e2 & PTE_ADDR_MASK);
    uint64_t e3 = pd[pd_index(virt_addr)];
    if ((e3 & VMM_PTE_P) == 0) return 0;
    if ((e3 & VMM_PTE_PS) != 0) return 0;

    uint64_t *pt = (uint64_t *) phys_to_virt(e3 & PTE_ADDR_MASK);
    uint64_t e4 = pt[pt_index(virt_addr)];
    if ((e4 & VMM_PTE_P) == 0) return 0;

    uint64_t phys_page = e4 & PTE_ADDR_MASK;
    uint64_t offset = virt_addr & 0xFFFull;
    return phys_page + offset;
}
