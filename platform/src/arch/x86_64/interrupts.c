#include "interrupts.h"
#include "cpu_interrupts.h"

#include <stdint.h>

#include "boot.h"
#include "gdt.h"
#include "io_wrapper.h"
#include "lapic.h"
#include "scheduler.h"
#include "drivers/serial.h"

// The IDT structure
struct idt_entry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static irq_handler_t handlers[256];
static void *handler_priv[256];

void interrupts_init(void) {
    // This table must be defined in your NASM file (see below)
    extern uint64_t interrupt_stubs[];

    for (int i = 0; i < 256; i++) {
        uint64_t isr = interrupt_stubs[i];

        idt[i].isr_low = (uint16_t) (isr & 0xFFFF);
        idt[i].kernel_cs = KERNEL_CODE_SEGMENT;
        idt[i].ist = 0; // No specific stack switching
        uint8_t attributes;
        if (i == SYSCALL_INTERRUPT_NUMBER) {
            // Allow to be manually triggerred from ring 3
            attributes = 0xEE;
        } else {
            attributes = 0x8E; // Present, Ring 0, Interrupt Gate
        }
        idt[i].attributes = attributes;
        idt[i].isr_mid = (uint16_t) ((isr >> 16) & 0xFFFF);
        idt[i].isr_high = (uint32_t) ((isr >> 32) & 0xFFFFFFFF);
        idt[i].reserved = 0;
    }

    struct idt_ptr ptr = {
        .limit = sizeof(idt) - 1,
        .base = (uint64_t) &idt
    };

    __asm__ volatile("lidt %0" : : "m"(ptr));

    interrupts_enable();
}

void interrupts_enable(void) {
    __asm__ volatile("sti");
}

void interrupts_disable(void) {
    __asm__ volatile("cli");
}

bool interrupts_register_handler(uint32_t irq, irq_handler_t handler, void *priv) {
    if (irq < 0x30 || irq > 0xFF) return false; // allow APIC only (and we are mapping APIC from 0x30 to 0xFF)

    handlers[irq] = handler;
    handler_priv[irq] = priv;

    return true;
}

// The C dispatcher called from NASM
// Returns a pointer to a stack the assembly code should switch to
// If frame pointer == returned address, then the interrupt returns exactly where it was interrupted
uintptr_t x86_64_interrupt_dispatcher(struct interrupt_frame *frame) {
    if (frame->error_code != 0 || frame->interrupt_number < 0x20) {
        // non-zero error code and/or exceptions
        serial_print("Interrupt: ");
        serial_print_hex_u64(frame->interrupt_number);
        serial_println("");
        serial_print("Error code received: ");
        serial_print_hex_u64(frame->error_code);
        serial_println("");
        hcf();
    }
    struct interrupt_frame *return_frame = frame;

    if (frame->interrupt_number < 0x30) {
        serial_print("WARNING: legacy PIC interrupt");
        serial_print_hex_u64(frame->interrupt_number);
        serial_println("");
    }

    if (handlers[frame->interrupt_number]) {
        handlers[frame->interrupt_number](&return_frame, handler_priv[frame->interrupt_number]);
    } else {
        serial_print("Unhandled interrupt: ");
        serial_print_hex_u64(frame->interrupt_number);
        serial_println("");
    }

    if (frame->interrupt_number >= 0x20 && frame->interrupt_number <= 0x2F) {
        if (frame->interrupt_number >= 0x28) {
            outb(0xA0, 0x20); // Send EOI to Slave PIC
        }
        outb(0x20, 0x20); // Send EOI to Master PIC
    }

    if (0x30 <= frame->interrupt_number && frame->interrupt_number < 0x80) {
        // APIC interrupts
        lapic_eoi();
    }

    return (uintptr_t) return_frame;
}
