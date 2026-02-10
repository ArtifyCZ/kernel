#pragma once

#include <stdint.h>

typedef void (*thread_fn_t)(void *arg);

typedef void (*thread_trampoline_t)(thread_fn_t fn, void *arg);

struct thread_ctx;

struct thread_ctx *thread_setup(
    uintptr_t stack_top,
    thread_trampoline_t trampoline,
    thread_fn_t fn,
    void *arg
);

void thread_context_switch(struct thread_ctx **old_ctx, struct thread_ctx *new_ctx);
