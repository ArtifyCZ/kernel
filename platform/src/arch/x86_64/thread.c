#include "thread.h"
#include "thread_priv.h"

#include "gdt.h"
#include "physical_memory_manager.h"
#include "string.h"
#include "virtual_memory_manager.h"

#define USER_SPACE_CODE 0x400000
#define USER_SPACE_STACK 0x7FFFFFFFF000

struct thread_ctx *thread_setup(
    uintptr_t kernel_stack_top,
    thread_fn_t fn,
    void *arg,
    bool is_user_mode
) {
    // 1. Point to the very bottom of the frame on the kernel stack
    uintptr_t sp = kernel_stack_top - sizeof(struct interrupt_frame);
    struct interrupt_frame *frame = (struct interrupt_frame *) sp;

    // Zero out the frame to avoid junk in registers
    memset(frame, 0, sizeof(struct interrupt_frame));

    if (is_user_mode) {
        // --- Hardware Frame (Pushed by CPU) ---
        frame->ss = USER_DATA_SEGMENT | 3; // 0x1B
        frame->rsp = (USER_SPACE_STACK & ~0xFULL);
        frame->rflags = 0x202; // Interrupts enabled
        frame->cs = USER_CODE_SEGMENT | 3; // 0x23
        frame->rip = USER_SPACE_CODE;

        // --- Software Frame (Pushed by your stub) ---
        frame->rdi = (uintptr_t) arg; // First argument in SysV ABI
        struct vmm_context user_ctx = vmm_context_create();
        frame->cr3 = user_ctx.root;

        // --- Memory Mapping ---
        // Map the function 'fn' to the user's virtual code address
        uintptr_t fn_phys = vmm_translate(&g_kernel_context, (uintptr_t) fn);
        vmm_map_page(
            &user_ctx,
            USER_SPACE_CODE,
            fn_phys,
            VMM_FLAG_PRESENT | VMM_FLAG_USER | VMM_FLAG_EXEC
        );

        // Map the user stack (remember stack grows DOWN)
        uintptr_t stack_phys = pmm_alloc_frame();
        vmm_map_page(
            &user_ctx,
            USER_SPACE_STACK - 0x1000,
            stack_phys,
            VMM_FLAG_PRESENT | VMM_FLAG_USER | VMM_FLAG_WRITE
        );
    } else {
        // --- Kernel Mode logic ---
        frame->ss = KERNEL_DATA_SEGMENT;
        frame->rsp = kernel_stack_top;
        frame->rflags = 0x202;
        frame->cs = KERNEL_CODE_SEGMENT;
        frame->rip = (uintptr_t) fn;
        frame->rdi = (uintptr_t) arg;
        frame->cr3 = g_kernel_context.root;
    }

    // Return the new stack pointer for the scheduler to store
    return (struct thread_ctx *) sp;
}
