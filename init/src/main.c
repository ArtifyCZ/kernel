#include "syscalls.h"

void print(const char *message) {
    size_t length = 0;
    while (message[length] != '\0') {
        length++;
    }

    sys_write(1, message, length);
}

void rest(void) {
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
}

__attribute__ ((noreturn)) void second_thread(void) {
    print("Hello from second thread!\n");
    rest();
    sys_exit();
    print("THIS SHOULD NOT HAPPEN!");
    while (1) {}
}

int main(void) {
    const char message[] = "Hello world from user-space!\n";
    print(message);

    print("Trying to invoke clone syscall...\n");
    // @TODO: add stack allocation
    sys_clone(0, (void *) 0x7FFFFFFF8000, second_thread);
    print("Parent is moving on...\n");

    rest();
    return 0;
}
