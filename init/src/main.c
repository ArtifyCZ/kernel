#include "syscalls.h"

void _start(void) {
    const char message[] = "Hello world from user-space!\n";

    while (1) {
        for (volatile int i = 0; i < 100000000; i++);
        sys_write(1, message, sizeof(message) - 1);
    }
}
