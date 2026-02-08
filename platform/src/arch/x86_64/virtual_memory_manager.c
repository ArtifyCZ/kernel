#include "virtual_memory_manager.h"
#include "virtual_memory_manager_priv.h"
#include "physical_memory_manager.h"
#include "string.h"

static uint64_t g_hhdm_offset = 0;
struct vmm_context g_kernel_context;

// Internal helpers
static inline void* get_vaddr(uintptr_t phys) {
    return (void*)(phys + g_hhdm_offset);
}

static inline void invlpg(uintptr_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// Convert our generic VMM flags to x86 hardware bits
static uint64_t flags_to_x86(vmm_flags_t flags) {
    uint64_t x86_flags = 0;
    if (flags & VMM_FLAG_PRESENT) x86_flags |= X86_PTE_PRESENT;
    if (flags & VMM_FLAG_WRITE)   x86_flags |= X86_PTE_WRITE;
    if (flags & VMM_FLAG_USER)    x86_flags |= X86_PTE_USER;
    if (!(flags & VMM_FLAG_EXEC)) x86_flags |= X86_PTE_NX;

    // For Device memory (MMIO) on x86, we usually disable caching
    if (flags & VMM_FLAG_DEVICE || flags & VMM_FLAG_NOCACHE) {
        x86_flags |= X86_PTE_PCD | X86_PTE_PWT;
    }
    return x86_flags;
}

void vmm_init(uint64_t hhdm_offset) {
    g_hhdm_offset = hhdm_offset;

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));

    // x86_64 CR3 contains the physical address of the PML4
    g_kernel_context.root = cr3 & X86_ADDR_MASK;
}

bool vmm_map_page(struct vmm_context *context, uintptr_t virt, uintptr_t phys, vmm_flags_t flags) {
    static const int shifts[] = {39, 30, 21, 12};
    uint64_t* table = get_vaddr(context->root);
    uint64_t hw_flags = flags_to_x86(flags);

    for (int i = 0; i < 3; i++) {
        int idx = (virt >> shifts[i]) & 0x1FF;
        if (!(table[idx] & X86_PTE_PRESENT)) {
            uintptr_t new_table_phys = pmm_alloc_frame();
            if (!new_table_phys) return false;

            // Note: Use your implemented memset here!
            memset(get_vaddr(new_table_phys), 0, VMM_PAGE_SIZE);

            // Link the new table. Intermediate levels need P and W
            // to allow the leaf entry to control permissions.
            table[idx] = new_table_phys | X86_PTE_PRESENT | X86_PTE_WRITE;
        }
        table = get_vaddr(table[idx] & X86_ADDR_MASK);
    }

    int idx = (virt >> shifts[3]) & 0x1FF;
    table[idx] = (phys & X86_ADDR_MASK) | hw_flags;

    invlpg(virt);
    return true;
}

bool vmm_unmap_page(struct vmm_context *context, uintptr_t virt) {
    static const int shifts[] = {39, 30, 21, 12};
    uint64_t* table = get_vaddr(context->root);

    for (int i = 0; i < 3; i++) {
        int idx = (virt >> shifts[i]) & 0x1FF;
        if (!(table[idx] & X86_PTE_PRESENT)) return false;
        table = get_vaddr(table[idx] & X86_ADDR_MASK);
    }

    table[(virt >> shifts[3]) & 0x1FF] = 0;
    invlpg(virt);
    return true;
}

uintptr_t vmm_translate(struct vmm_context *context, uintptr_t virt) {
    static const int shifts[] = {39, 30, 21, 12};
    uint64_t* table = get_vaddr(context->root);

    for (int i = 0; i < 3; i++) {
        int idx = (virt >> shifts[i]) & 0x1FF;
        uint64_t entry = table[idx];
        if (!(entry & X86_PTE_PRESENT)) return 0;

        // Handle huge pages (PS bit) if encountered
        if (entry & X86_PTE_PS) {
            uintptr_t mask = (1ULL << shifts[i]) - 1;
            return (entry & X86_ADDR_MASK & ~mask) | (virt & mask);
        }
        table = get_vaddr(entry & X86_ADDR_MASK);
    }

    uint64_t entry = table[(virt >> 12) & 0x1FF];
    if (!(entry & X86_PTE_PRESENT)) return 0;

    return (entry & X86_ADDR_MASK) | (virt & 0xFFF);
}
