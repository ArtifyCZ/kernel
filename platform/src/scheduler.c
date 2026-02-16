#include "scheduler.h"

#include "drivers/serial.h"
#include "interrupts.h"
#include "stddef.h"

#define MAX_THREADS  8
#define STACK_SIZE   (16 * 1024)

enum thread_state {
    T_UNUSED = 0,
    T_RUNNABLE = 1,
    T_RUNNING = 2,
    T_DEAD = 3,
};

struct thread {
    enum thread_state state;
    struct thread_ctx *ctx; // Now a pointer to the context on the stack

    // stack lives in kernel BSS
    uint8_t stack[STACK_SIZE] __attribute__((aligned(16)));
} __attribute__((aligned(16)));

static struct thread g_threads[MAX_THREADS];
static int g_current = -1;

_Noreturn void sched_start(void) {
    serial_println("sched: starting");

#if defined (__x86_64__)
    __asm__ volatile ("int $0x30"); // @TODO: replace this by a proper thread yield syscall or something
#endif

    // Should never reach here on x86_64
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__)
        asm ("wfi");
#endif
    }
}
