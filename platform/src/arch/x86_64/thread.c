#include "thread.h"
#include "thread_priv.h"

#include "gdt.h"

struct thread_ctx *thread_setup(
    uintptr_t stack_top,
    thread_trampoline_t trampoline,
    thread_fn_t fn,
    void *arg
) {
    // Ensure stack_top starts 16-byte aligned
    uintptr_t sp = stack_top & ~0xFULL;

    // Reserve space for the thread_ctx struct
    // This effectively places the 'rip' at the highest address
    sp -= sizeof(struct thread_ctx);

    struct thread_ctx *ctx = (struct thread_ctx *) sp;

    // Initialize the "pushed" registers
    // We use r12, r13, and rbx to carry data into arch_thread_entry
    ctx->frame.r15 = 0;
    ctx->frame.r14 = 0;
    ctx->frame.r13 = (uintptr_t) arg; // Passed to rsi in arch_thread_entry
    ctx->frame.r12 = (uintptr_t) fn; // Passed to rdi in arch_thread_entry
    ctx->frame.rbp = 0;
    ctx->frame.rbx = (uintptr_t) trampoline; // Called in arch_thread_entry

    // The 'ret' in thread_context_switch will pop this into RIP
    ctx->frame.rip = (uintptr_t) arch_thread_entry;
    ctx->frame.rflags = 0x202; // interrupt flag set, others default

    ctx->frame.cs = KERNEL_CODE_SEGMENT;
    ctx->frame.ss = KERNEL_DATA_SEGMENT;

    ctx->frame.rsp = stack_top;

    return ctx;
}
