#include "virtual_memory_manager.h"
#include "virtual_memory_manager_priv.h"
#include "physical_memory_manager.h"
#include "string.h"

struct vmm_context g_kernel_context;
static uint64_t g_hhdm = 0;

// Internal helper to get a virtual pointer from a physical address via HHDM
static inline uint64_t* get_vaddr(uintptr_t paddr) {
    return (uint64_t*)(paddr + g_hhdm);
}

// Translate generic VMM flags to AArch64 descriptor bits
static uint64_t flags_to_desc(vmm_flags_t flags) {
    uint64_t desc = DESC_VALID | DESC_AF | DESC_SH_INNER;

    if (!(flags & VMM_FLAG_WRITE)) desc |= DESC_RO;
    if (flags & VMM_FLAG_USER)    desc |= DESC_USER;

    // Execute Never bits: PXN for kernel, UXN for user
    if (!(flags & VMM_FLAG_EXEC)) {
        desc |= DESC_PXN;
        if (flags & VMM_FLAG_USER) desc |= DESC_UXN;
    }

    // Memory Attributes (MAIR)
    if (flags & VMM_FLAG_DEVICE || flags & VMM_FLAG_NOCACHE) {
        desc |= (ATTR_DEVICE_IDX << 2);
    } else {
        desc |= (ATTR_NORMAL_IDX << 2);
    }

    return desc;
}

static void init_mair(void) {
    uint64_t mair = 0;

    // Attribute 0: Device-nGnRnE (Strongly Ordered)
    // Used for MMIO (UART, Interrupt Controller, etc.)
    mair |= (0x00ULL << (ATTR_DEVICE_IDX * 8));

    // Attribute 1: Normal Memory, Outer Write-Back Non-transient
    // Used for RAM (Kernel code, stack, heap)
    mair |= (0xFFULL << (ATTR_NORMAL_IDX * 8));

    __asm__ volatile("msr mair_el1, %0" : : "r"(mair));
}

void vmm_init(uint64_t hhdm_offset) {
    g_hhdm = hhdm_offset;

    init_mair();

    // Capture the existing root from TTBR1 (Kernel half)
    uintptr_t current_root;
    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(current_root));

    // Mask out the ASID and other non-address bits
    g_kernel_context.root = current_root & 0x0000FFFFFFFFF000ULL;
}


bool vmm_map_page(struct vmm_context *context, uintptr_t virt, uintptr_t phys, vmm_flags_t flags) {
    uint64_t* table = get_vaddr(context->root);
    static const int shifts[] = {L0_SHIFT, L1_SHIFT, L2_SHIFT};

    for (int i = 0; i < 3; i++) {
        int idx = (virt >> shifts[i]) & IDX_MASK;

        if (!(table[idx] & DESC_VALID)) {
            uintptr_t new_table_phys = pmm_alloc_frame();
            if (!new_table_phys) return false;

            memset(get_vaddr(new_table_phys), 0, VMM_PAGE_SIZE);
            table[idx] = new_table_phys | DESC_TABLE | DESC_VALID;
        }
        table = get_vaddr(table[idx] & 0x0000FFFFFFFFF000ULL);
    }

    int l3_idx = (virt >> L3_SHIFT) & IDX_MASK;
    table[l3_idx] = (phys & 0x0000FFFFFFFFF000ULL) | DESC_PAGE | flags_to_desc(flags);

    // Invalidate TLB for this specific address (global/inner-shareable)
    __asm__ volatile("tlbi vaae1is, %0; dsb sy; isb" : : "r"(virt >> 12));

    return true;
}

bool vmm_unmap_page(struct vmm_context *context, uintptr_t virt) {
    uint64_t* table = get_vaddr(context->root);
    const static int shifts[] = {L0_SHIFT, L1_SHIFT, L2_SHIFT};

    for (int i = 0; i < 3; i++) {
        int idx = (virt >> shifts[i]) & IDX_MASK;
        if (!(table[idx] & DESC_VALID)) return false;
        table = get_vaddr(table[idx] & 0x0000FFFFFFFFF000ULL);
    }

    int l3_idx = (virt >> L3_SHIFT) & IDX_MASK;
    table[l3_idx] = 0;

    __asm__ volatile("tlbi vaae1is, %0; dsb sy; isb" : : "r"(virt >> 12));
    return true;
}

uintptr_t vmm_translate(struct vmm_context *context, uintptr_t virt) {
    uint64_t* table = get_vaddr(context->root);
    const static int shifts[] = {L0_SHIFT, L1_SHIFT, L2_SHIFT, L3_SHIFT};
    volatile int foo = 5;
    (void) foo;

    for (int i = 0; i < 4; i++) {
        int idx = (virt >> shifts[i]) & IDX_MASK;
        uint64_t entry = table[idx];

        if (!(entry & DESC_VALID)) return 0;

        // Check if this is a terminal page (L3) or a block mapping (L1/L2)
        if (i == 3 || (i > 0 && !(entry & DESC_TABLE))) {
            uintptr_t page_mask = (1ULL << shifts[i]) - 1;
            uintptr_t paddr_mask = 0x0000FFFFFFFFF000ULL & ~page_mask;
            return (entry & paddr_mask) | (virt & page_mask);
        }
        table = get_vaddr(entry & 0x0000FFFFFFFFF000ULL);
    }
    return 0;
}