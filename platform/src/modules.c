//
// Created by artify on 2/4/26.
//

#include "modules.h"

#include "drivers/serial.h"
#include "stdbool.h"
#include "stddef.h"

static struct limine_module_response *modules = NULL;

static inline bool strcmp(const char *a, const char *b) {
    size_t i;
    for (i = 0; a[i] != 0x00 && b[i] != 0x00; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return a[i] == b[i];
}

void modules_init(struct limine_module_response *modules_arg) {
    serial_println("Initializing modules...");
    modules = modules_arg;

    for (size_t i = 0; modules != NULL && i < modules->module_count; i++) {
        const struct limine_file *file = modules->modules[i];
        serial_print("Module ");
        serial_print(": ");
        serial_print(file->string);
        serial_print(" at ");
        serial_println(file->path);
    }
}

const struct limine_file *module_find(const char *associated_string) {
    for (size_t i = 0; modules != NULL && i < modules->module_count; i++) {
        const struct limine_file *file = modules->modules[i];
        if (strcmp(associated_string, file->string)) {
            return file;
        }
    }

    return NULL;
}
