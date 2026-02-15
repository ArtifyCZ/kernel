#pragma once

#include "syscalls.h"
#include "syscalls_priv.h"

#include "cpu_interrupts.h"
#include <stdbool.h>

void syscalls_init() {
    // Do nothing
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
    const uint64_t return_value = syscalls_dispatch(&syscall_frame);
    previous_frame->x[0] = return_value;

    return true;
}
