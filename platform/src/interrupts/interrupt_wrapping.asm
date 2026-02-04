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

%macro pic_irq_wrap 2
isr_wrap_%+%2:
    push 0
    push %2
    jmp common_interrupt_handler
%endmacro


common_interrupt_handler:
    ; Save ALL GPRs (interrupts can happen anywhere; must preserve full context)
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rbx
    push rdx
    push rcx
    push rax

    ; Ensure 16-byte alignment for the C call.
    ; At this point, pushes above are 15*8 = 120 bytes => misaligns by 8.
    sub rsp, 8

    lea rdi, [rsp + 8]   ; pass pointer to the saved-register frame (skip alignment pad)
    call interrupt_handler

    add rsp, 8

    ; Restore ALL GPRs
    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16          ; pop interrupt_number + error_code (or vector + CPU error code)
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
pic_irq_wrap 0,32
pic_irq_wrap 1,33
pic_irq_wrap 2,34
pic_irq_wrap 3,35
pic_irq_wrap 4,36
pic_irq_wrap 5,37
pic_irq_wrap 6,38
pic_irq_wrap 7,39
pic_irq_wrap 8,40
pic_irq_wrap 9,41
pic_irq_wrap 10,42
pic_irq_wrap 11,43
pic_irq_wrap 12,44
pic_irq_wrap 13,45
pic_irq_wrap 14,46
pic_irq_wrap 15,47

global isr_stub_table
isr_stub_table:
%assign i 0
%rep    47
    dq isr_wrap_%+i ; use DQ instead if targeting 64-bit
%assign i i+1
%endrep
