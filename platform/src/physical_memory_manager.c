//
// Created by artify on 2/3/26.
//

#include "physical_memory_manager.h"

#include "drivers/serial.h"

static inline uint64_t align_up_u64(uint64_t v, uint64_t a) {
    return (v + (a - 1)) & ~(a - 1);
}

static inline uint64_t align_down_u64(uint64_t v, uint64_t a) {
    return v & ~(a - 1);
}

#define PAGE_FRAMES_STACK_CAPACITY 0x100000 // up to 1024^2 page frames

static uintptr_t page_frames[PAGE_FRAMES_STACK_CAPACITY];
static size_t page_frames_count;

static void push_page_frame(const uintptr_t page_frame) {
    if (page_frames_count == PAGE_FRAMES_STACK_CAPACITY) {
        serial_println("Cannot push more frames onto stack, the stack is full!");
        return;
    }

    page_frames[page_frames_count++] = page_frame;
}

static uintptr_t pop_page_frame(void) {
    if (page_frames_count == 0) {
        serial_println("Cannot pop from empty stack!");
        return 0; // NULL
    }

    return page_frames[--page_frames_count];
}

void pmm_init(struct limine_memmap_response *memmap) {
    page_frames_count = 0;
    if (memmap == NULL) {
        serial_println("Got NULL ptr as memmap response; cannot initialize PPM");
        return;
    }

    for (size_t i = 0; i < memmap->entry_count; i++) {
        const struct limine_memmap_entry *memmap_entry = memmap->entries[i];
        if (memmap_entry == NULL) {
            serial_println("Got NULL ptr as memmap entry, continuing");
            continue;
        }

        if (memmap_entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        uint64_t start = align_up_u64(memmap_entry->base, PPM_PAGE_SIZE);
        uint64_t end = align_down_u64(memmap_entry->base + memmap_entry->length, PPM_PAGE_SIZE);
        for (uint64_t addr = start; addr < end; addr += PPM_PAGE_SIZE) {
            push_page_frame(addr);
        }
    }
}

uintptr_t pmm_alloc_frame(void) {
    return pop_page_frame();
}

bool pmm_free_frame(uintptr_t physical_frame_address) {
    if (physical_frame_address == 0x0) {
        serial_println("Trying to free NULL ptr as physical frame address!");
        return false;
    }

    if (physical_frame_address % PPM_PAGE_SIZE != 0) {
        serial_println("Trying to free non-aligned physical frame address!");
        return false;
    }

    push_page_frame(physical_frame_address);
    return true;
}

size_t pmm_get_available_frames_count(void) {
    return page_frames_count;
}
