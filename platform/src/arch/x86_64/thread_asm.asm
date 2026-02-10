[bits 64]

section .text

global thread_context_switch
global arch_thread_entry

; void thread_context_switch(struct thread_ctx **old_ctx_ptr, struct thread_ctx *new_ctx)
; rdi = pointer to the 'ctx' field in the old thread struct
; rsi = the value of the 'ctx' field from the new thread struct
thread_context_switch:
    ; 1. Save callee-saved registers onto current stack
    ; rip is already at [rsp] because of the 'call' instruction
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; 2. Save the current RSP into the old_ctx_ptr
    mov [rdi], rsp

    ; 3. Switch the stack
    mov rsp, rsi

    ; 4. Restore registers from the new stack
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ; 5. This pops the 'rip' from the new stack and jumps to it
    ret

arch_thread_entry:
    ; Values were placed in these registers by thread_setup
    ; We move them to rdi/rsi to follow the C calling convention
    mov rdi, r12    ; arg 1: thread_fn_t fn
    mov rsi, r13    ; arg 2: void *arg
    call rbx        ; call the trampoline(fn, arg)

    ; Should never return
    cli
.loop:
    hlt
    jmp .loop
