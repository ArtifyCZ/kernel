#include "thread.h"
#include "thread_priv.h"

#include "physical_memory_manager.h"
#include "string.h"
#include "virtual_memory_manager.h"

struct thread_ctx *thread_setup_user(
    struct vmm_context *user_ctx,
    const uintptr_t entrypoint_vaddr,
    const uintptr_t kernel_stack_top
) {
    uintptr_t sp = (kernel_stack_top & ~0xFULL) - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    // Zero the frame
    memset(frame, 0, sizeof(struct interrupt_frame));

    uintptr_t user_stack_top = 0x7FFFFFFFF000; // @TODO: add rather some logic for choosing the stack address
    for (size_t i = 0; i < 4; i++) {
        // Allocate and map 4 pages for stack
        uintptr_t page_virt = user_stack_top - (i + 1) * VMM_PAGE_SIZE;
        uintptr_t page_phys = pmm_alloc_frame();

        vmm_map_page(user_ctx, page_virt, page_phys, VMM_FLAG_PRESENT | VMM_FLAG_USER | VMM_FLAG_WRITE);
    }

    // SPSR_EL1:
    // M[3:0] = 0000 (Return to EL0t)
    // Bit 6,7,8,9 = 0 (Unmask Debug, SError, IRQ, FIQ)
    frame->spsr = 0x00;

    // Point to our User Virtual Addresses
    frame->ttbr0 = user_ctx->root;
    frame->elr = entrypoint_vaddr;
    frame->sp_el0 = (user_stack_top & ~0xFULL);

    return (struct thread_ctx *) sp;
}

struct thread_ctx *thread_setup_kernel(
    uintptr_t kernel_stack_top,
    thread_fn_t fn,
    void *arg
) {
    uintptr_t sp = (kernel_stack_top & ~0xFULL) - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    // Zero the frame
    memset(frame, 0, sizeof(struct interrupt_frame));

    // SPSR_EL1: M[3:0] = 0101 (Return to EL1h)
    frame->spsr = 0x05 | (0 << 6) | (0 << 7);
    frame->elr = (uintptr_t) fn;
    frame->ttbr0 = g_kernel_context.root; // ttbr0 is used only in EL0, not EL1 (user-space vs. kernel)

    frame->x[0] = (uintptr_t) arg;

    return (struct thread_ctx *) sp;
}

void thread_prepare_switch(uintptr_t kernel_stack_top) {
    // Do nothing
}
