#pragma once

#include "thread.h"

void sched_init(void);

int sched_create(thread_fn_t fn, void *arg);

_Noreturn void sched_start(void);

void sched_request_reschedule(void);

void sched_yield_if_needed(void);

_Noreturn void sched_exit(void);
