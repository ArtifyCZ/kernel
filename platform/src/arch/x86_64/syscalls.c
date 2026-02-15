#include "syscalls.h"

#include <stddef.h>
#include "cpu_interrupts.h"

#define SYSCALL_INTERRUPT_VECTOR 0x80

bool syscalls_interrupt_handler(struct interrupt_frame **frame, void *args);

void syscalls_init(void) {
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
    uint64_t return_value = syscalls_dispatch(&syscall_frame);
    previous_frame->rax = return_value;

    return true;
}
