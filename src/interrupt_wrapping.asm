%macro isr_err_wrap 1
isr_wrap_%+%1:
    push %1
    jmp common_interrupt_handler
%endmacro

%macro isr_no_err_wrap 1
isr_wrap_%+%1:
    push 0        ; push 0 as error code
    push %1       ; push interrupt number as error code
    jmp common_interrupt_handler
%endmacro



common_interrupt_handler:
    ; save the registers
    push   rax
    push   rcx
    push   rdx
    push   r8
    push   r9
    push   r10
    push   r11

    cli                 ; disable interrupts

    mov rdi, rsp        ; pass the stack pointer as the first argument to interrupt_handler
    call interrupt_handler

    ; restore the registers
    pop    r11
    pop    r10
    pop    r9
    pop    r8
    pop    rdx
    pop    rcx
    pop    rax

    add rsp, 16         ; pop interrupt_number + error_code

    sti                 ; enable interrupts

    iretq


extern interrupt_handler

isr_no_err_wrap 0
isr_no_err_wrap 1
isr_no_err_wrap 2
isr_no_err_wrap 3
isr_no_err_wrap 4
isr_no_err_wrap 5
isr_no_err_wrap 6
isr_no_err_wrap 7
isr_err_wrap    8
isr_no_err_wrap 9
isr_err_wrap    10
isr_err_wrap    11
isr_err_wrap    12
isr_err_wrap    13
isr_err_wrap    14
isr_no_err_wrap 15
isr_no_err_wrap 16
isr_err_wrap    17
isr_no_err_wrap 18
isr_no_err_wrap 19
isr_no_err_wrap 20
isr_no_err_wrap 21
isr_no_err_wrap 22
isr_no_err_wrap 23
isr_no_err_wrap 24
isr_no_err_wrap 25
isr_no_err_wrap 26
isr_no_err_wrap 27
isr_no_err_wrap 28
isr_no_err_wrap 29
isr_err_wrap    30
isr_no_err_wrap 31

global isr_stub_table
isr_stub_table:
%assign i 0
%rep    32
    dq isr_wrap_%+i ; use DQ instead if targeting 64-bit
%assign i i+1
%endrep
