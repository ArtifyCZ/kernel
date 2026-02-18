#include "tasks.h"

#include "cpu_interrupts.h"
#include <stdint.h>
#include "string.h"
#include "virtual_memory_manager.h"

typedef void (*kernel_task_fn_t)(void *arg);

struct interrupt_frame *task_setup_user(
    const struct vmm_context *user_ctx,
    const uintptr_t entrypoint_vaddr,
    const uintptr_t user_stack_top,
    const uintptr_t kernel_stack_top
) {
    const uintptr_t sp = (kernel_stack_top & ~0xFULL) - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    memset(frame, 0, sizeof(struct interrupt_frame));

    // SPSR_EL1:
    // M[3:0] = 0000 (Return to EL0t)
    // Bit 6,7,8,9 = 0 (Unmask Debug, SError, IRQ, FIQ)
    frame->spsr = 0x00;

    frame->ttbr0 = user_ctx->root;
    frame->elr = entrypoint_vaddr;
    frame->sp_el0 = (user_stack_top & ~0xFULL);

    return (struct interrupt_frame *) sp;
}

struct interrupt_frame *task_setup_kernel(
    const uintptr_t stack_top,
    const kernel_task_fn_t fn,
    void *arg
) {
    const uintptr_t sp = (stack_top & ~0xFULL) - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    memset(frame, 0, sizeof(struct interrupt_frame));

    // SPSR_EL1: M[3:0] = 0101 (Return to EL1h)
    frame->spsr = 0x05 | (0 << 6) | (0 << 7);
    frame->elr = (uintptr_t) fn;
    frame->ttbr0 = g_kernel_context.root; // ttbr0 is used only in EL0, not EL1 (user-space vs. kernel)

    frame->x[0] = (uintptr_t) arg;

    return (struct interrupt_frame *) sp;
}

void task_prepare_switch(uintptr_t kernel_stack_top) {
    // Do nothing
}
