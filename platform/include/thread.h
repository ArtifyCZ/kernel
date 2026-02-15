#pragma once

#include <stdint.h>
#include "virtual_memory_manager.h"

typedef void (*thread_fn_t)(void *arg);

typedef void (*thread_trampoline_t)(thread_fn_t fn, void *arg);

struct thread_ctx;

struct thread_ctx *thread_setup_user(
    struct vmm_context *user_ctx,
    uintptr_t entrypoint_vaddr,
    uintptr_t kernel_stack_top
);

struct thread_ctx *thread_setup_kernel(
    uintptr_t stack_top,
    thread_fn_t fn,
    void *arg
);
