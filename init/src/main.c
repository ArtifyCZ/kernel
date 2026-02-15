static inline void syscall() {
#if defined(__x86_64__)
    __asm__ volatile ("int $0x80");
#elif defined(__aarch64__)
    __asm__ volatile ("svc 0");
#endif
}

void _start(void) {
    syscall();

    while (1) {
        for (volatile int i = 0; i < 100000000; i++);
        syscall();
    }
}
