#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "multiboot.h"
#include "cga_graphics.h"
#include "cpuid.h"

bool machine_supports_64bit(void) {
    uint32_t cpuidResults[4];

    cpuid(CPUIDExtendedFunctionSupport, 0, cpuidResults);

    if (cpuidResults[CPUID_EAX] < CPUIDExtendedProcessorInfo) {
        return false;
    }

    cpuid(CPUIDExtendedProcessorInfo, 0, cpuidResults);

    return cpuidResults[CPUID_EDX] & (1 << 29);
}

void kernel_main(multiboot_info_t* multiboot_info) {
    cga_clear_screen();

    cga_print_string("i'm a kernel", 0, 0);

    if (multiboot_info->flags & MULTIBOOT_INFO_CMDLINE) {
        cga_print_string("cmdline: ", 0, 1);
        cga_print_string((const char*)multiboot_info->cmdline, 9, 1);
    }

    if (multiboot_info->flags & MULTIBOOT_INFO_MODS && multiboot_info->mods_count > 0) {
        const multiboot_module_t* module = (const multiboot_module_t*)multiboot_info->mods_addr;

        cga_print_string("module: ", 0, 2);
        cga_print_string((const char*)module->mod_start, 8, 2);
    }

    if (!cpuid_supported()) {
        cga_print_string("cpuid unsupported", 0, 3);
        return;
    }

    cga_print_string("64-bit: ", 0, 3);
    cga_print_string(machine_supports_64bit() ? "supported" : "unsupported", 8, 3);

    // returning here will result in our 'hang' routine executing
}
