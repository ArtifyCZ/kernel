#pragma once

#include "thread.h"

#include <stdint.h>

// AArch64 Callee-saved context (must be 16-byte aligned total)
struct thread_ctx {
    uint64_t x19, x20; // [sp, #0]
    uint64_t x21, x22; // [sp, #16]
    uint64_t x23, x24; // [sp, #32]
    uint64_t x25, x26; // [sp, #48]
    uint64_t x27, x28; // [sp, #64]
    uint64_t fp,  lr;  // [sp, #80] (x29, x30)
    uint64_t padding;
    uint64_t align;
} __attribute__((aligned(16)));

extern void arch_thread_entry(void);
