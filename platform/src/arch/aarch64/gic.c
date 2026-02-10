#include "gic.h"

#include <stdint.h>

#include "virtual_address_allocator.h"
#include "virtual_memory_manager.h"

// Simplified GICv2 initialization for QEMU virt
#define GIC_DIST_BASE 0x08000000
#define GIC_CPU_BASE  0x08010000

static uint64_t g_gic_dist_base = 0;

static uint64_t g_gic_cpu_base = 0;

void gic_init(void) {
    g_gic_dist_base = vaa_alloc_range(VMM_PAGE_SIZE);
    g_gic_cpu_base = vaa_alloc_range(VMM_PAGE_SIZE);

    vmm_map_page(
        &g_kernel_context,
        g_gic_dist_base,
        GIC_DIST_BASE,
        VMM_FLAG_PRESENT | VMM_FLAG_WRITE | VMM_FLAG_DEVICE
    );

    vmm_map_page(
        &g_kernel_context,
        g_gic_cpu_base,
        GIC_CPU_BASE,
        VMM_FLAG_PRESENT | VMM_FLAG_WRITE | VMM_FLAG_DEVICE
    );

    // 1. Enable Distributor: Bit 0 = Group 0, Bit 1 = Group 1
    *(volatile uint32_t *) (g_gic_dist_base + 0x000) = 0x3;

    // 2. Enable CPU Interface:
    // Bit 0: Enable Group 0
    // Bit 1: Enable Group 1
    // Bit 2: AckCtl (Allows Group 1 to be acked)
    // Bit 3: FIQEn (Optional, routes Group 0 as FIQ)
    // Bit 4: CBPR (Common Binary Point Register)
    *(volatile uint32_t *) (g_gic_cpu_base + 0x000) = 0x1F; // Enable all logic

    // 3. Set Priority Mask to allow all interrupts
    *(volatile uint32_t *) (g_gic_cpu_base + 0x004) = 0xFF;
}

void gic_enable_interrupt(uint32_t vector) {
    // Each bit in GICD_ISENABLERn enables one interrupt
    uint32_t reg = vector / 32;
    uint32_t bit = vector % 32;
    *(volatile uint32_t*)(g_gic_dist_base + 0x100 + (reg * 4)) = (1 << bit);
}

void gic_configure_interrupt(uint32_t vector, uint8_t priority) {
    // 1. Set Priority (GICD_IPRIORITYRn)
    uint32_t prio_reg = vector / 4;
    uint32_t prio_off = (vector % 4) * 8;

    uint32_t val = *(volatile uint32_t*)(g_gic_dist_base + 0x400 + (prio_reg * 4));
    val &= ~(0xFF << prio_off);
    val |= (priority << prio_off);
    *(volatile uint32_t*)(g_gic_dist_base + 0x400 + (prio_reg * 4)) = val;

    // 2. Set Group 1 (GICD_IGROUPRn)
    uint32_t group_reg = vector / 32;
    uint32_t group_bit = vector % 32;
    // Use |= to preserve other interrupt group settings in the same register
    *(volatile uint32_t*)(g_gic_dist_base + 0x080 + (group_reg * 4)) |= (1 << group_bit);
}

uint32_t gic_acknowledge_interrupt(void) {
    return *(volatile uint32_t *) (g_gic_cpu_base + 0x00C) & 0x3FF;
}

void gic_end_of_interrupt(const uint32_t id) {
    *(volatile uint32_t *) (g_gic_cpu_base + 0x010) = id;
}
