//
// Created by artify on 2/1/26.
//

#ifndef KERNEL_2026_01_31_BOOT_H
#define KERNEL_2026_01_31_BOOT_H

// Halt and catch fire function.
_Noreturn void hcf(void);

// The following will be our kernel's entry point.
// If renaming boot() to something else, make sure to change the
// linker script accordingly.
__attribute__((used)) void boot(void);

extern void kernel_main(void);

#endif //KERNEL_2026_01_31_BOOT_H
