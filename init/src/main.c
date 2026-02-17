#include "syscalls.h"

void print(const char *message) {
    size_t length = 0;
    while (message[length] != '\0') {
        length++;
    }

    sys_write(1, message, length);
}

void _start(void) {
    const char message[] = "Hello world from user-space!\n";
    print(message);
    int j = 0;

    while (1) {
        for (volatile int i = 0; i < 10000000; i++);
        char number = j + '0';
        sys_write(1, &number, 1);
        char newline = '\n';
        sys_write(1, &newline, 1);
        j++;
        if (j == 10) {
            break;
        }
    }
    print("Trying to invoke exit syscall...\n");
    sys_exit();
    print("THIS SHOULD NOT HAPPEN!\n");
    while (1) {
    }
}
