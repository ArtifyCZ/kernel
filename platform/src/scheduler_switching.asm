global context_switch
global thread_entry

extern sched_exit

; struct thread_ctx layout (must match scheduler.h):
; 0x00 rsp
; 0x08 rbx
; 0x10 rbp
; 0x18 r12
; 0x20 r13
; 0x28 r14
; 0x30 r15

context_switch:
    cli ; disable interrupts

    ; rdi = old_ctx*, rsi = new_ctx*
    mov [rdi + 0x00], rsp
    mov [rdi + 0x08], rbx
    mov [rdi + 0x10], rbp
    mov [rdi + 0x18], r12
    mov [rdi + 0x20], r13
    mov [rdi + 0x28], r14
    mov [rdi + 0x30], r15

    mov rsp, [rsi + 0x00]
    mov rbx, [rsi + 0x08]
    mov rbp, [rsi + 0x10]
    mov r12, [rsi + 0x18]
    mov r13, [rsi + 0x20]
    mov r14, [rsi + 0x28]
    mov r15, [rsi + 0x30]

    sti ; enable interrupts

    ret

; New thread starts here after the first context_switch "ret"
; Stack at entry:
;   [rsp+0] = fn
;   [rsp+8] = arg
thread_entry:
    pop rbx        ; fn
    pop rdi        ; arg (1st arg in SysV)
    call rbx
    call sched_exit
.hang:
    hlt
    jmp .hang
