#pragma once

#include "thread.h"

#include "cpu_interrupts.h"
#include <stdint.h>

struct thread_ctx {
    struct interrupt_frame frame;
};

extern void arch_thread_entry(void);
