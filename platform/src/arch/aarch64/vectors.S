/* vectors.S - AArch64 Exception Vector Table */

.section .text
.align 11 /* The vector table must be aligned to a 2KB (2^11) boundary */

.global exception_vector_table
exception_vector_table:
    /* --- Current EL with SP0 (Unused by most kernels) --- */
    .align 7
    b sync_handler_stub
    .align 7
    b irq_handler_stub
    .align 7
    b fiq_handler_stub
    .align 7
    b serror_handler_stub

    /* --- Current EL with SPx (This is what we use in EL1) --- */
    .align 7
    b sync_handler_stub      /* Offset 0x200 */
    .align 7
    b irq_handler_stub       /* Offset 0x280 */
    .align 7
    b fiq_handler_stub       /* Offset 0x300 */
    .align 7
    b serror_handler_stub    /* Offset 0x380 */

    /* --- Lower EL using AArch64 --- */
    .align 7
    b sync_handler_stub
    .align 7
    b irq_handler_stub
    .align 7
    b fiq_handler_stub
    .align 7
    b serror_handler_stub

    /* --- Lower EL using AArch32 --- */
    .align 7
    b sync_handler_stub
    .align 7
    b irq_handler_stub
    .align 7
    b fiq_handler_stub
    .align 7
    b serror_handler_stub

/* --- Context Saving Macro --- */
.macro push_context
    /* 272 bytes = (34 registers * 8 bytes).
       Includes x0-x30, SPSR, ELR, ESR.
       Already 16-byte aligned. */
    sub sp, sp, #272

    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]

    mrs x0, spsr_el1
    mrs x1, elr_el1
    mrs x2, esr_el1
    str x30, [sp, #240]
    str x0,  [sp, #248]
    str x1,  [sp, #256]
    str x2,  [sp, #264]

    mov x0, sp /* Frame pointer as 1st arg for C */
.endm

/* --- Context Restoration Macro --- */
.macro pop_context
    /* Restore control registers */
    ldr x0, [sp, #248]
    ldr x1, [sp, #256]
    msr spsr_el1, x0
    msr elr_el1, x1

    /* Restore general purpose registers */
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]

    add sp, sp, #272
    eret
.endm

/* --- Stubs --- */
.type sync_handler_stub, %function
sync_handler_stub:
    push_context
    bl handle_sync_exception
    pop_context

.type irq_handler_stub, %function
irq_handler_stub:
    push_context
    bl handle_irq_exception
    pop_context

.type fiq_handler_stub, %function
fiq_handler_stub:
    push_context
    bl handle_fiq_exception
    pop_context

.type serror_handler_stub, %function
serror_handler_stub:
    push_context
    bl handle_serror_exception
    pop_context
