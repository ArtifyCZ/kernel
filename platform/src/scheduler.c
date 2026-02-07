//
// Created by Richard Tichý on 04.02.2026.
//

#include "scheduler.h"

#include "boot.h"
#include "drivers/serial.h"
#include "stdbool.h"
#include "terminal.h"
#include "interrupts.h"

#define MAX_THREADS  8
#define STACK_SIZE   (16 * 1024)

enum thread_state {
    T_UNUSED = 0,
    T_RUNNABLE,
    T_RUNNING,
    T_DEAD
};

struct thread {
    enum thread_state state;
    struct thread_ctx ctx;

    // stack lives in kernel BSS for now (simple, predictable)
    uint8_t stack[STACK_SIZE] __attribute__((aligned(16)));
};

static struct thread g_threads[MAX_THREADS];
static int g_current = -1;

static volatile bool g_need_resched = false;

static struct thread_ctx boot_ctx;

void sched_request_reschedule(void) {
    g_need_resched = true;
}

void sched_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        g_threads[i].state = T_UNUSED;
        g_threads[i].ctx.rsp = 0;
    }
    g_current = -1;
    g_need_resched = false;

    serial_println("sched: initialized");
}

static int pick_next_runnable(void) {
    if (g_current < 0) {
        for (int i = 0; i < MAX_THREADS; i++) {
            if (g_threads[i].state == T_RUNNABLE) return i;
        }
        return -1;
    }

    for (int off = 1; off <= MAX_THREADS; off++) {
        int idx = (g_current + off) % MAX_THREADS;
        if (g_threads[idx].state == T_RUNNABLE) return idx;
    }
    return -1;
}

int sched_create(thread_fn_t fn, void *arg) {
    disable_interrupts();

    int idx = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].state == T_UNUSED) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        enable_interrupts();
        return -1;
    }

    struct thread *t = &g_threads[idx];

    // Build initial stack so that context_switch() "ret" lands in thread_entry.
    uintptr_t sp = (uintptr_t) &t->stack[STACK_SIZE];

    // Ensure 16-byte alignment before a call boundary.
    sp &= ~(uintptr_t) 0xF;

    sp -= sizeof(uint64_t);
    *(uint64_t *) sp = (uint64_t) (uintptr_t) arg;

    sp -= sizeof(uint64_t);
    *(uint64_t *) sp = (uint64_t) (uintptr_t) fn;

    // Stack layout (top -> bottom):
    //   [return RIP] = thread_entry
    //   [fn]         = fn
    //   [arg]        = arg
    sp -= sizeof(uint64_t);
    *(uint64_t *) sp = (uint64_t) (uintptr_t) thread_entry;

    t->ctx.rsp = sp;
    t->ctx.rbx = 0;
    t->ctx.rbp = 0;
    t->ctx.r12 = 0;
    t->ctx.r13 = 0;
    t->ctx.r14 = 0;
    t->ctx.r15 = 0;

    t->state = T_RUNNABLE;

    enable_interrupts();
    return idx;
}

static void sched_yield_now(void) {
    int next = pick_next_runnable();
    if (next < 0) return;
    if (next == g_current) return;

    int prev = g_current;
    g_current = next;

    if (prev >= 0) {
        g_threads[prev].state = T_RUNNABLE;
    }
    g_threads[next].state = T_RUNNING;

    if (prev < 0) {
        context_switch(&boot_ctx, &g_threads[next].ctx);
        return;
    }

    context_switch(&g_threads[prev].ctx, &g_threads[next].ctx);
}

void sched_yield_if_needed(void) {
    if (!g_need_resched) return;

    disable_interrupts();
    if (!g_need_resched) {
        enable_interrupts();
        return;
    }
    g_need_resched = false;
    sched_yield_now();
    enable_interrupts();
}

_Noreturn void sched_exit(void) {
    disable_interrupts();

    if (g_current >= 0) {
        g_threads[g_current].state = T_DEAD;
    }

    // Keep switching to whatever is runnable.
    for (;;) {
        sched_yield_now();
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

_Noreturn void sched_start(void) {
    disable_interrupts();
    serial_println("sched: starting");
    g_need_resched = false;
    sched_yield_now();
    enable_interrupts();

    hcf();
}
