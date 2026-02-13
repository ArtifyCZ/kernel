[bits 64]

section .text

global arch_thread_entry

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
