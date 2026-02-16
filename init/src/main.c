#include "syscalls.h"

void _start(void) {
    const char message[] = "Hello world from user-space!\n";
    sys_write(1, message, sizeof(message) - 1);
    int j = 0;

    while (1) {
        for (volatile int i = 0; i < 100000000; i++);
        char number = j + '0';
        sys_write(1, &number, 1);
        char newline = '\n';
        sys_write(1, &newline, 1);
        j++;
        j = j % 10;
    }
}
