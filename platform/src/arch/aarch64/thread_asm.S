.global thread_context_switch
.global arch_thread_entry

// void thread_context_switch(struct thread_ctx **old_sp_ptr, struct thread_ctx *new_sp)
// x0 = &old_thread->sp  (Pointer to the pointer in the struct)
// x1 = new_thread->sp   (The actual pointer value we want to load)
thread_context_switch:
    // 1. Push callee-saved registers onto the CURRENT stack
    // 112 bytes for 16-byte alignment (saving 14 registers: x19-x30, plus padding/future)
    sub sp, sp, #112

    stp x19, x20, [sp, #0]
    stp x21, x22, [sp, #16]
    stp x23, x24, [sp, #32]
    stp x25, x26, [sp, #48]
    stp x27, x28, [sp, #64]
    stp x29, x30, [sp, #80]

    // 2. Save the resulting Stack Pointer into the 'old_sp_ptr'
    // This updates the struct thread's 'sp' field so we know where to resume later
    mov x2, sp
    str x2, [x0]

    // 3. SWITCH STACK: Load the NEW Stack Pointer
    // x1 already contains the address of the context, so no 'ldr' from memory needed
    mov sp, x1

    // 4. Pop registers from the NEW stack
    ldp x19, x20, [sp, #0]
    ldp x21, x22, [sp, #16]
    ldp x23, x24, [sp, #32]
    ldp x25, x26, [sp, #48]
    ldp x27, x28, [sp, #64]
    ldp x29, x30, [sp, #80]

    add sp, sp, #112

    // 5. Return
    // If this is a new thread, x30 was initialized to arch_thread_entry
    ret

arch_thread_entry:
    // Arguments were loaded into x19-x21 by the context switch 'ldp' instructions
    // from the stack frame initialized in thread_setup.
    mov x0, x20    // fn
    mov x1, x21    // arg
    br x19         // jump to the trampoline (scheduler_wrapper)
