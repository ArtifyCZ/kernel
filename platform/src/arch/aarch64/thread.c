#include "thread.h"
#include "thread_priv.h"

struct thread_ctx *thread_setup(
    uintptr_t stack_top,
    thread_trampoline_t trampoline,
    thread_fn_t fn,
    void *arg
) {
    // Align and carve space
    uintptr_t stack_addr = (stack_top & ~0xFULL) - sizeof(struct thread_ctx);
    struct thread_ctx *ctx = (struct thread_ctx *) stack_addr;

    // Initialize
    ctx->frame.x[19] = (uintptr_t) trampoline;
    ctx->frame.x[20] = (uintptr_t) fn;
    ctx->frame.x[21] = (uintptr_t) arg;

    ctx->frame.elr = (uintptr_t) arch_thread_entry;
    // SPSR_EL1: M[3:0] = 0101 (EL1h), interrupts enabled (DAIF = 0)
    // This tells 'eret' to stay in EL1 but use the SP_EL1 stack.
    ctx->frame.spsr = 0x05;

    return ctx; // Hand the "handle" back to the scheduler
}
