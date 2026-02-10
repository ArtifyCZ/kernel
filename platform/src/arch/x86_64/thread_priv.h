#pragma once

#include "thread.h"

#include <stdint.h>

struct thread_ctx {
    // Callee-saved registers (pushed by our ASM)
    uint64_t r15;   // [rsp + 0]
    uint64_t r14;   // [rsp + 8]
    uint64_t r13;   // [rsp + 16]
    uint64_t r12;   // [rsp + 24]
    uint64_t rbp;   // [rsp + 32]
    uint64_t rbx;   // [rsp + 40]

    // The return address (pushed by the 'call' to thread_context_switch)
    uint64_t rip;   // [rsp + 48]
} __attribute__((aligned(16)));

extern void arch_thread_entry(void);
