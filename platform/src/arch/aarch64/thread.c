#include "thread.h"
#include "thread_priv.h"

struct thread_ctx *thread_setup(
    uintptr_t stack_top,
    thread_trampoline_t trampoline,
    thread_fn_t fn,
    void *arg
) {
    // 1. Align and carve space
    uintptr_t stack_addr = (stack_top & ~0xFULL) - sizeof(struct thread_ctx);
    struct thread_ctx *ctx = (struct thread_ctx *) stack_addr;

    // 2. Initialize
    ctx->x19 = (uintptr_t) trampoline;
    ctx->x20 = (uintptr_t) fn;
    ctx->x21 = (uintptr_t) arg;

    // Clear other callee-saved regs for predictability
    ctx->x22 = ctx->x23 = ctx->x24 = ctx->x25 = ctx->x26 = ctx->x27 = ctx->x28 = 0;

    ctx->fp = 0;
    ctx->lr = (uintptr_t) arch_thread_entry;

    return ctx; // Hand the "handle" back to the scheduler
}
