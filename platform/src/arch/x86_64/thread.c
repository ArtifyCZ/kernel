#include "thread.h"
#include "thread_priv.h"

struct thread_ctx *thread_setup(
    uintptr_t stack_top,
    thread_trampoline_t trampoline,
    thread_fn_t fn,
    void *arg
) {
    // 1. Ensure stack_top starts 16-byte aligned
    uintptr_t sp = stack_top & ~0xFULL;

    // 2. Reserve space for the thread_ctx struct
    // This effectively places the 'rip' at the highest address
    sp -= sizeof(struct thread_ctx);

    struct thread_ctx *ctx = (struct thread_ctx *) sp;

    // 3. Initialize the "pushed" registers
    // We use r12, r13, and rbx to carry data into arch_thread_entry
    ctx->r15 = 0;
    ctx->r14 = 0;
    ctx->r13 = (uintptr_t) arg; // Passed to rsi in arch_thread_entry
    ctx->r12 = (uintptr_t) fn; // Passed to rdi in arch_thread_entry
    ctx->rbp = 0;
    ctx->rbx = (uintptr_t) trampoline; // Called in arch_thread_entry

    // 4. The 'ret' in thread_context_switch will pop this into RIP
    ctx->rip = (uintptr_t) arch_thread_entry;

    return ctx;
}
