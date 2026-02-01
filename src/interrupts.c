//
// Created by artify on 2/1/26.
//

#include "interrupts.h"
#include "stdint.h"
#include "stdbool.h"
#include "serial.h"
#include "boot.h"

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

#define IDT_MAX_DESCRIPTORS 32

static bool vectors[IDT_MAX_DESCRIPTORS];

extern void *isr_stub_table[];

void interrupt_handler(struct stack_frame *frame) {
    serial_println("Interrupt received!");
}

static struct idtr idtr;

void idt_init(void) {
    __asm__ volatile ("mov %%cs, %0" : "=r"(g_kernel_cs));

    idtr.base = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) (sizeof(struct idt_entry) * IDT_MAX_DESCRIPTORS - 1);

    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag

    serial_println("Initialized interrupts");
}
