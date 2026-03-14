#include "libs/libsyscall/syscalls.h"

void print(const char *message) {
    size_t length = 0;
    while (message[length] != '\0') {
        length++;
    }

    sys_write(1, message, length);
}


__attribute__((noreturn)) void _start(void) {
    print("Hello world from initrd-loaded program!");
    sys_exit();
    while (1);
}
