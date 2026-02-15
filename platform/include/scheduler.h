#pragma once

#include "interrupts.h"
#include "thread.h"

void sched_init(void);

int sched_create_user(struct vmm_context *user_ctx, uintptr_t entrypoint_vaddr);

int sched_create_kernel(thread_fn_t fn, void *arg);

_Noreturn void sched_start(void);

void sched_request_reschedule(void);

// Returns an interrupt frame the interrupt should return into
struct thread_ctx *sched_heartbeat(struct thread_ctx *frame);

void sched_set_current_thread_dead(void);

_Noreturn void sched_exit(void);
