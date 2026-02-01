//
// Created by artify on 2/1/26.
//

#ifndef KERNEL_2026_01_31_BOOT_H
#define KERNEL_2026_01_31_BOOT_H

#include <limine.h>

extern struct limine_framebuffer *framebuffer;

// The following will be our kernel's entry point.
// If renaming bootEntrypoint() to something else, make sure to change the
// linker script accordingly.
__attribute__((used)) void bootEntrypoint(void);

#endif //KERNEL_2026_01_31_BOOT_H
