#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "interrupts/interrupts.h"
#include "boot.h"

#include "keyboard.h"
#include "modules.h"
#include "physical_memory_manager.h"
#include "pit.h"
#include "psf.h"
#include "scheduler.h"
#include "serial.h"
#include "terminal.h"
#include "virtual_memory_manager.h"

// Set the base revision to 4, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0
};


// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// Halt and catch fire function.
_Noreturn void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

void try_virtual_mapping(void) {
    const uintptr_t physical_frame = pmm_alloc_frame();
    if (physical_frame == 0x0) {
        serial_println("Cannot allocate physical frame for virtual mapping!");
        return;
    }
    const uintptr_t virtual_address = 0xFFFFC00000000000;
    if (vmm_translate(virtual_address) != 0x0) {
        serial_println("Cannot map virtual address 0xFFFFC00000000000, address already mapped!");
        return;
    }

    if (!vmm_map_page(virtual_address, physical_frame, VMM_PTE_W)) {
        serial_println("Cannot map virtual address 0xFFFFC00000000000!");
        return;
    }

    serial_println("Virtual mapping successful!");
    serial_println("Trying to write to the mapped memory");

    volatile uint64_t *ptr = (volatile uint64_t *) virtual_address;
    ptr[0] = 0x1122334455667788ull;
    ptr[1] = 0xA5A5A5A5A5A5A5A5ull;

    serial_println("Done!");

    if (ptr[0] != 0x1122334455667788ull || ptr[1] != 0xA5A5A5A5A5A5A5A5ull) {
        serial_println("VMM test: readback mismatch");
        (void) vmm_unmap_page(virtual_address);
        pmm_free_frame(physical_frame);
        return;
    }

    (void) vmm_unmap_page(virtual_address);
    pmm_free_frame(physical_frame);

    serial_println("VMM test: success!");
}

static void thread_heartbeat(void *arg) {
    (void)arg;

    for (;;) {
        terminal_print_char('.');
        for (volatile uint64_t i = 0; i < 2000000; i++) { }
        sched_yield_if_needed();
    }
}

static void thread_keyboard(void *arg) {
    (void)arg;

    for (;;) {
        uint8_t sc;
        if (!keyboard_read_char(&sc)) {
            sched_yield_if_needed();
            continue;
        }

        char c = kbd_translate_scancode(sc);
        if (c == '\0') {
            sched_yield_if_needed();
            continue;
        }

        terminal_print_char(c);
        sched_yield_if_needed();
    }
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
__attribute__((used)) void boot(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    serial_init();

    if (memmap_request.response == NULL) {
        serial_println("Limine memmap missing; cannot init PMM");
        hcf();
    }

    pmm_init(memmap_request.response);

    if (hhdm_request.response == NULL) {
        serial_println("Limine HHDM missing; cannot init VMM");
        hcf();
    }

    vmm_init(hhdm_request.response->offset);

    try_virtual_mapping();

    idt_init();

    serial_println("Trying to invoke an interrupt");
#if defined (__x86_64__)
    __asm__ volatile ("int $0x0");
#elif defined (__aarch64__) || defined (__riscv)
    asm ("svc 0");
#else
#error Architecture not supported
#endif
    serial_println("Interrupt invoked successfully!");

    modules_init(module_request.response);

    const struct limine_file *font = module_find("kernel-font.psf");

    if (font != NULL) {
        psf_init(font->address, font->size, framebuffer_request.response->framebuffers[0]);
        terminal_init(framebuffer_request.response->framebuffers[0]);
    } else {
        serial_println("Could not find kernel-font.psf!");
    }

    terminal_set_foreground_color(0xD4DBDF);
    terminal_set_background_color(0x04121B);
    terminal_clear();

    terminal_println("Hello world!");

    sched_init();

    (void)sched_create(thread_keyboard, NULL);
    (void)sched_create(thread_heartbeat, NULL);

    pit_init(100);

    sched_start();

    kernel_main();

    // We're done, just hang...
    hcf();
}
