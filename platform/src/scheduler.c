#include "scheduler.h"

#include "drivers/serial.h"
#include "stdbool.h"
#include "interrupts.h"
#include "stddef.h"
#include "arch/x86_64/gdt.h"

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

void sched_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        g_threads[i].state = T_UNUSED;
        g_threads[i].ctx = NULL;
    }
    g_current = -1;

    serial_println("sched: initialized");
}

static int pick_next_runnable(void) {
    for (int off = 1; off <= MAX_THREADS; off++) {
        int idx = (g_current < 0) ? (off - 1) : (g_current + off) % MAX_THREADS;
        if (g_threads[idx].state == T_RUNNABLE)
            return idx;
    }
    return -1;
}

int sched_create(thread_fn_t fn, void *arg, bool is_user_space) {
    interrupts_disable();

    int idx = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].state == T_UNUSED) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        interrupts_enable();
        return -1;
    }

    struct thread *t = &g_threads[idx];
    uintptr_t stack_top = (uintptr_t) &t->stack[STACK_SIZE];

    // Use the agnostic thread_setup.
    // It returns the pointer to the context it carved out of the stack.
    t->ctx = thread_setup(stack_top, fn, arg, is_user_space);
    t->state = T_RUNNABLE;

    interrupts_enable();
    return idx;
}

struct thread_ctx *sched_heartbeat(struct thread_ctx *frame) {
    // @TODO: use proper types instead of converting to (void *) to specific
    int next = pick_next_runnable();
    if (next < 0)
        return frame;
    if (next == g_current)
        return frame;

    int prev = g_current;
    g_current = next;

    if (prev >= 0) {
        g_threads[prev].state = T_RUNNABLE;
        g_threads[prev].ctx = (void *) frame;
    }
    g_threads[next].state = T_RUNNING;
#if defined (__x86_64__)
    gdt_set_kernel_stack((uintptr_t)&g_threads[next].stack[STACK_SIZE]);
#endif
    return g_threads[next].ctx;
}

_Noreturn void sched_exit(void) {
    if (g_current >= 0) {
        g_threads[g_current].state = T_DEAD;
    }

    for (;;) {
        // If nothing is runnable, wait for an interrupt (IDLE state)
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__)
        asm ("wfi");
#endif
    }
}

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
