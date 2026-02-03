//
// Created by artify on 2/3/26.
//

#ifndef KERNEL_2026_01_31_VIRTUAL_MEMORY_MANAGER_H
#define KERNEL_2026_01_31_VIRTUAL_MEMORY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define VMM_PAGE_SIZE 0x1000ull

// Page table flags (x86_64)
#define VMM_PTE_P   (1ull << 0)   // Present
#define VMM_PTE_W   (1ull << 1)   // Writable
#define VMM_PTE_U   (1ull << 2)   // User
#define VMM_PTE_PWT (1ull << 3)
#define VMM_PTE_PCD (1ull << 4)
#define VMM_PTE_A   (1ull << 5)
#define VMM_PTE_D   (1ull << 6)
#define VMM_PTE_PS  (1ull << 7)   // Large page (not used for 4KiB mappings)
#define VMM_PTE_G   (1ull << 8)
#define VMM_PTE_NX  (1ull << 63)  // No-execute

// Initialize VMM using Limine HHDM offset (virt = phys + offset).
void vmm_init(uint64_t hhdm_offset);

// Map/unmap a single 4KiB page in the current address space (CR3).
// phys_addr must be 4KiB-aligned; virt_addr must be 4KiB-aligned.
bool vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
bool vmm_unmap_page(uint64_t virt_addr);

// Translate a virtual address to physical (returns 0 if not mapped).
uint64_t vmm_translate(uint64_t virt_addr);

#endif //KERNEL_2026_01_31_VIRTUAL_MEMORY_MANAGER_H
