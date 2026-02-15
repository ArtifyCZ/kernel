#include "thread.h"
#include "thread_priv.h"

#include "gdt.h"
#include "physical_memory_manager.h"
#include "string.h"
#include "virtual_memory_manager.h"

#define USER_SPACE_CODE 0x400000
#define USER_SPACE_STACK 0x7FFFFFFFF000

struct thread_ctx *thread_setup_user(
    struct vmm_context *user_ctx,
    const uintptr_t entrypoint_vaddr,
    const uintptr_t kernel_stack_top
) {
    // Point to the very bottom of the frame on the kernel stack
    uintptr_t sp = kernel_stack_top - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    // Zero out the frame to avoid junk in registers
    memset(frame, 0, sizeof(struct interrupt_frame));

    uintptr_t user_stack_top = 0x7FFFFFFFF000; // @TODO: add rather some logic for choosing the stack address
    for (size_t i = 0; i < 4; i++) {
        // Allocate and map 4 pages for stack
        uintptr_t page_virt = user_stack_top - (i + 1) * VMM_PAGE_SIZE;
        uintptr_t page_phys = pmm_alloc_frame();

        vmm_map_page(user_ctx, page_virt, page_phys, VMM_FLAG_PRESENT | VMM_FLAG_USER | VMM_FLAG_WRITE);
    }

    // --- Hardware Frame (Pushed by CPU) ---
    frame->ss = USER_DATA_SEGMENT | 3; // 0x1B
    frame->rsp = (user_stack_top & ~0xFULL);
    frame->rflags = 0x202; // Interrupts enabled
    frame->cs = USER_CODE_SEGMENT | 3; // 0x23
    frame->rip = entrypoint_vaddr;

    frame->cr3 = user_ctx->root;

    // Return the new stack pointer for the scheduler to store
    return (struct thread_ctx *) sp;
}

struct thread_ctx *thread_setup_kernel(
    uintptr_t kernel_stack_top,
    thread_fn_t fn,
    void *arg
) {
    // Point to the very bottom of the frame on the kernel stack
    uintptr_t sp = kernel_stack_top - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    // Zero out the frame to avoid junk in registers
    memset(frame, 0, sizeof(struct interrupt_frame));

    frame->ss = KERNEL_DATA_SEGMENT;
    frame->rsp = kernel_stack_top;
    frame->rflags = 0x202;
    frame->cs = KERNEL_CODE_SEGMENT;
    frame->rip = (uintptr_t) fn;
    frame->rdi = (uintptr_t) arg;
    frame->cr3 = g_kernel_context.root;

    // Return the new stack pointer for the scheduler to store
    return (struct thread_ctx *) sp;
}
