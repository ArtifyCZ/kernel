#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "interrupts.h"
#include "boot.h"
#include "main.h"
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
static volatile struct limine_module_request module_request = {
        .id = LIMINE_MODULE_REQUEST_ID,
        .revision = 0
};

size_t strlen(const char *string) {
    size_t i = 1;
    while (string[i] != 0x00) {
        i++;
    }

    return i;
}

bool stringEndsWith(const char *string, const char *suffix) {
    const char *string_idx = string + strlen(string) - 1;
    const char *suffix_idx = suffix + strlen(suffix) - 1;
    while (string <= string_idx && suffix <= suffix_idx) {
        if (*string_idx != *suffix_idx) {
            return false;
        }
        string_idx--;
        suffix_idx--;
    }
    return true;
}

struct limine_file *getFile(const char *name) {
    for (size_t i = 0; i < module_request.response->module_count; i++) {
        struct limine_file *file = module_request.response->modules[i];
        if (stringEndsWith(file->path, name)) {
            return file;
        }
    }

    return NULL;
}



// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// Halt and catch fire function.
static void hcf(void) {
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

struct limine_framebuffer *framebuffer;

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
__attribute__((unused)) void bootEntrypoint(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
        || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    framebuffer = framebuffer_request.response->framebuffers[0];

    idt_init();

    serial_init();

    main();

    // We're done, just hang...
    hcf();
}
