#pragma once

#include "syscalls.h"
#include "syscalls_priv.h"

#include "cpu_interrupts.h"
#include <stdbool.h>
#include <stddef.h>

static syscall_handler_t g_syscall_handler = NULL;
static void *g_syscall_handler_arg = NULL;

void syscalls_init(syscall_handler_t syscall_handler, void *syscall_handler_arg) {
    g_syscall_handler = syscall_handler;
    g_syscall_handler_arg = syscall_handler_arg;
}

bool syscalls_interrupt_handler(struct interrupt_frame **frame) {
    struct syscall_frame syscall_frame = {
        .interrupt_frame = frame,
        .num = (*frame)->x[8],
        .a = {
            (*frame)->x[0],
            (*frame)->x[1],
            (*frame)->x[2],
            (*frame)->x[3],
            (*frame)->x[4],
        },
    };

    struct interrupt_frame *previous_frame = *frame;
    const uint64_t return_value = g_syscall_handler(&syscall_frame, g_syscall_handler_arg);
    previous_frame->x[0] = return_value;

    return true;
}
