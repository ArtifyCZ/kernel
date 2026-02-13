#include "thread.h"
#include "thread_priv.h"

#include "physical_memory_manager.h"
#include "string.h"
#include "virtual_memory_manager.h"

#define USER_SPACE_CODE 0x400000
#define USER_SPACE_STACK 0x7FFFFFFFF000

struct thread_ctx *thread_setup(
    uintptr_t kernel_stack_top,
    thread_fn_t fn,
    void *arg,
    bool is_user_space
) {
    uintptr_t sp = (kernel_stack_top & ~0xFULL) - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    // Zero the frame
    memset(frame, 0, sizeof(struct interrupt_frame));

    if (is_user_space) {
        // --- Userspace Path ---
        // SPSR_EL1:
        // M[3:0] = 0000 (Return to EL0t)
        // Bit 6,7,8,9 = 0 (Unmask Debug, SError, IRQ, FIQ)
        frame->spsr = 0x00;

        struct vmm_context user_ctx = vmm_context_create();

        // Point to our User Virtual Addresses
        frame->ttbr0 = user_ctx.root;
        frame->elr = USER_SPACE_CODE;
        frame->sp_el0 = USER_SPACE_STACK;

        // Pass 'arg' via x0 (Standard ARM calling convention)
        frame->x[0] = (uintptr_t) arg;

        // VMM: Map the kernel function 'fn' to user space (similar to x86)
        // Ensure you use aarch64 specific flags (User Accessible, Execute allowed)
        vmm_map_page(
            &user_ctx,
            USER_SPACE_CODE,
            vmm_translate(&g_kernel_context, (uintptr_t) fn),
            VMM_FLAG_PRESENT | VMM_FLAG_USER | VMM_FLAG_EXEC
        );

        // VMM: Map User Stack
        vmm_map_page(
            &user_ctx,
            USER_SPACE_STACK - 0x1000,
            pmm_alloc_frame(),
            VMM_FLAG_PRESENT | VMM_FLAG_USER | VMM_FLAG_WRITE
        );
    } else {
        // --- Kernel Path ---
        // SPSR_EL1: M[3:0] = 0101 (Return to EL1h)
        frame->spsr = 0x05;
        frame->elr = (uintptr_t) arch_thread_entry;
        frame->ttbr0 = 0x0; // ttbr0 is used only in EL0, not EL1 (user-space vs. kernel)

        // Pass params into x19-x21 for your trampoline
        frame->x[19] = (uintptr_t) arch_thread_entry; // Use your trampoline logic
        frame->x[20] = (uintptr_t) fn;
        frame->x[21] = (uintptr_t) arg;
    }

    return (struct thread_ctx *) sp;
}
