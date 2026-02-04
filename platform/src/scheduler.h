//
// Created by Richard Tichý on 04.02.2026.
//

#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <stdint.h>

typedef void (*thread_fn_t)(void *arg);

struct thread_ctx {
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} __attribute__((__packed__));

void sched_init(void);
int  sched_create(thread_fn_t fn, void *arg);
_Noreturn void sched_start(void);

void sched_request_reschedule(void);
void sched_yield_if_needed(void);
_Noreturn void sched_exit(void);

// ASM
void context_switch(struct thread_ctx *old_ctx, struct thread_ctx *new_ctx);
void thread_entry(void);

#endif //KERNEL_SCHEDULER_H
