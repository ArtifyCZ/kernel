#include "syscalls.h"

int main(void);

static void print(const char *message) {
    size_t length = 0;
    while (message[length] != '\0') {
        length++;
    }

    sys_write(1, message, length);
}

__attribute__((noreturn)) void _start(void) {
    print("Hello from _start!\n");
    int exit_code = main();
    print("Exiting...\n");
    sys_exit(); // @TODO: also pass the exit code
    while (1) {
    }
}
