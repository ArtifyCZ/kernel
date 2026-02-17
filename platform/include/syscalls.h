#pragma once

#include <stdint.h>
#include "interrupts.h"

struct syscall_frame {
    uint64_t num;
    uint64_t a[5];
    struct interrupt_frame **interrupt_frame;
};

void syscalls_init(void);

uint64_t syscalls_dispatch(struct syscall_frame *frame);
