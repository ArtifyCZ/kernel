//
// Created by artify on 2/1/26.
//

#include "interrupts.h"
#include "stdint.h"
#include "stdbool.h"
#include "../boot.h"
#include "../keyboard.h"
#include "../pic.h"
#include "../serial.h"
#include "../terminal.h"

__attribute__((aligned(0x10)))
static struct idt_entry idt[256]; // Create an array of IDT entries; aligned for performance

static uint16_t g_kernel_cs;

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    struct idt_entry *descriptor = &idt[vector];

    descriptor->isr_low = (uint64_t) isr & 0xFFFF;
    descriptor->kernel_cs = g_kernel_cs;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t) isr >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t) isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

#define IDT_MAX_DESCRIPTORS 47

static bool vectors[IDT_MAX_DESCRIPTORS];

extern void *isr_stub_table[];

static inline uint64_t read_cr2(void) {
    uint64_t v;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(v));
    return v;
}

void interrupt_handler(struct stack_frame *frame) {
    if (frame->interrupt_number >= 32) {
        pic_send_eoi(frame->interrupt_number - 32);
    }

    if (frame->interrupt_number == 0x20) {
        // This should be the timer IRQ0
        return;
    }

    if (frame->interrupt_number == 0x21) {
        // This should be the keyboard IRQ1
        kbd_read_and_push();
        return;
    }

    if (frame->interrupt_number == 0x0E) {
        // This should be the page fault
        const uint64_t cr2 = read_cr2();

        serial_println("");
        serial_println("=== PAGE FAULT (#PF) ===");
        serial_print("CR2 (fault addr): ");
        serial_print_hex_u64(cr2);
        serial_println("");

        serial_print("RIP: ");
        serial_print_hex_u64(frame->rip);
        serial_println("");

        serial_print("Error code: ");
        serial_print_hex_u64(frame->error_code);
        serial_println("");

        serial_print("Details: ");
        serial_print((frame->error_code & (1u << 0)) ? "protection " : "not-present ");
        serial_print((frame->error_code & (1u << 1)) ? "write " : "read ");
        serial_print((frame->error_code & (1u << 2)) ? "user " : "supervisor ");
        if (frame->error_code & (1u << 3)) serial_print("rsvd ");
        if (frame->error_code & (1u << 4)) serial_print("instr-fetch ");
        serial_println("");

        serial_println("Halting.");
        hcf();
    }

    serial_print("Interrupt received: ");
    serial_print_hex_u64(frame->interrupt_number);
    serial_println("");
}

static struct idtr idtr;

void idt_init(void) {
    __asm__ volatile ("cli"); // disable interrupts during idt initialization
    __asm__ volatile ("mov %%cs, %0" : "=r"(g_kernel_cs));

    pic_remap(0x20, 0x28);

    idtr.base = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) (sizeof(struct idt_entry) * IDT_MAX_DESCRIPTORS - 1);

    for (uint8_t vector = 0; vector < 47; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // enable interrupts, initialization is complete

    serial_println("Initialized interrupts");
}
