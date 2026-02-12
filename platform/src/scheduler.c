#include "scheduler.h"

#include "drivers/serial.h"
#include "stdbool.h"
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

static volatile bool g_need_resched = false;

// We need a place to store the "Main/Boot" thread's pointer
// when we switch away from it for the first time.
static struct thread_ctx *g_boot_ctx_ptr = NULL;

/**
 * The Trampoline: Every thread starts here.
 * This decouples the assembly from the scheduler's exit logic.
 */
static void scheduler_trampoline(thread_fn_t fn, void *arg) {
    // New threads start with interrupts disabled (from sched_start/yield)
    interrupts_enable();

    if (fn) {
        fn(arg);
    }

    sched_exit();
}

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
        if (g_threads[idx].state == T_RUNNABLE) return idx;
    }
    return -1;
}

int sched_create(thread_fn_t fn, void *arg) {
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
    t->ctx = thread_setup(stack_top, scheduler_trampoline, fn, arg);
    t->state = T_RUNNABLE;

    interrupts_enable();
    return idx;
}

static void sched_yield_now(void) {
    // @TODO: replace with a syscall to yield instead
    int next = pick_next_runnable();
    if (next < 0) return;
    if (next == g_current) return;

    int prev = g_current;
    g_current = next;

    if (prev >= 0) {
        g_threads[prev].state = T_RUNNABLE;
    }
    g_threads[next].state = T_RUNNING;

    // The logic:
    // &old_ptr: where the ASM will write the current SP
    // new_ptr:  the actual address the ASM will load into SP
    struct thread_ctx **old_ctx_ref;
    if (prev < 0) {
        old_ctx_ref = &g_boot_ctx_ptr;
    } else {
        old_ctx_ref = &g_threads[prev].ctx;
    }

    thread_context_switch(old_ctx_ref, g_threads[next].ctx);
}

struct thread_ctx *sched_heartbeat(struct thread_ctx *frame) {
    // @TODO: use proper types instead of converting to (void *) to specific
    int next = pick_next_runnable();
    if (next < 0) return frame;
    if (next == g_current) return frame;

    int prev = g_current;
    g_current = next;

    if (prev >= 0) {
        g_threads[prev].state = T_RUNNABLE;
        g_threads[prev].ctx = (void *) frame;
    }
    g_threads[next].state = T_RUNNING;
    return g_threads[next].ctx;
}

_Noreturn void sched_exit(void) {
    interrupts_disable();

    if (g_current >= 0) {
        g_threads[g_current].state = T_DEAD;
    }

    for (;;) {
        // Since current is now DEAD, yield_now will find a new T_RUNNABLE
        sched_yield_now();

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

    // This will switch from the caller's context into the first thread.
    // The caller's state will be saved in g_boot_ctx_ptr.
    // sched_yield_now();

    // Should never reach here
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__)
        asm ("wfi");
#endif
    }
}
