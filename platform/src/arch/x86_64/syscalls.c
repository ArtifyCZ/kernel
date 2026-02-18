#include "syscalls.h"

#include <stddef.h>
#include "cpu_interrupts.h"

#define SYSCALL_INTERRUPT_VECTOR 0x80

bool syscalls_interrupt_handler(struct interrupt_frame **frame, void *args);

static syscall_handler_t g_syscall_handler = NULL;
static void *g_syscall_handler_arg = NULL;

void syscalls_init(syscall_handler_t syscall_handler, void *syscall_handler_arg) {
    g_syscall_handler = syscall_handler;
    g_syscall_handler_arg = syscall_handler_arg;
    interrupts_register_handler(SYSCALL_INTERRUPT_VECTOR, syscalls_interrupt_handler, NULL);
}

bool syscalls_interrupt_handler(struct interrupt_frame **frame, void *args) {
    (void) args;

    struct syscall_frame syscall_frame = {
        .interrupt_frame = frame,
        .num = (*frame)->rax,
        .a = {
            (*frame)->rdi,
            (*frame)->rsi,
            (*frame)->rdx,
            (*frame)->r10,
            (*frame)->r8,
        },
    };

    struct interrupt_frame *previous_frame = *frame;
    const uint64_t return_value = g_syscall_handler(&syscall_frame, g_syscall_handler_arg);
    previous_frame->rax = return_value;

    return true;
}
