#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "interrupts.h"
#include "boot.h"

#include "physical_memory_manager.h"
#include "serial.h"

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

    pmm_init(memmap_request.response);

    serial_println("Trying to allocate some page frames");
    void *first_frame = pmm_alloc_frame();
    for (size_t i = 0; i < 10; i++) {
        serial_print(".");
        first_frame = pmm_alloc_frame();
    }
    serial_println("");
    serial_println("Done!");
    serial_println("");
    serial_println("Trying to free the first page frame");
    pmm_free_frame(first_frame);
    serial_println("Done!");
    size_t frames_count = pmm_get_available_frames_count();
    if (frames_count == 0) {
        serial_println("No frames left!");
    }

    idt_init();

    serial_println(module_request.response == NULL ? "No modules loaded." : "Modules loaded:");

    for (size_t i = 0; module_request.response != NULL && i < module_request.response->module_count; i++) {
        struct limine_file *file = module_request.response->modules[i];
        serial_print("Module ");
        serial_print(": ");
        serial_println(file->path);
    }

    kernel_main();

    // We're done, just hang...
    hcf();
}
