#pragma once

#include "interrupts.h"
#include "thread.h"

_Noreturn void sched_start(void);

// Returns an interrupt frame the interrupt should return into
struct thread_ctx *sched_heartbeat(struct thread_ctx *frame);
