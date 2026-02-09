#include "interrupts.h"
#include "io_wrapper.h"
#include "drivers/serial.h"
#include "stddef.h"
#include "timer.h"

#define PIT_CH0_DATA  0x40
#define PIT_CMD       0x43

bool pit_handle_irq0(struct interrupt_frame *frame, void *priv);

static volatile uint64_t g_ticks = 0;

static volatile timer_tick_handler_t g_tick_handler;

static volatile void *g_tick_handler_priv;

void timer_init(uint32_t freqHz) {
    if (freqHz == 0) freqHz = 100;

    g_ticks = 0;
    g_tick_handler = NULL;
    g_tick_handler_priv = NULL;
    // PIT input clock is ~1_193_182 Hz
    const uint32_t pit_base = 1193182u;
    uint16_t divisor = (uint16_t) (pit_base / freqHz);
    if (divisor == 0) divisor = 1;

    // Command: channel 0, access lobyte/hibyte, mode 3 (square wave), binary
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0_DATA, (uint8_t) (divisor & 0xFF));
    outb(PIT_CH0_DATA, (uint8_t) ((divisor >> 8) & 0xFF));

    // Remap PIC: IRQ 0-7 -> 0x20-0x27, IRQ 8-15 -> 0x28-0x2F
    outb(0x20, 0x11); outb(0xA0, 0x11); // Start initialization
    outb(0x21, 0x20); outb(0xA1, 0x28); // Set offset
    outb(0x21, 0x04); outb(0xA1, 0x02); // Tell Master/Slave about each other
    outb(0x21, 0x01); outb(0xA1, 0x01); // 8086 mode
    // Unmask IRQ0 (PIT) only: 0xFE = 11111110 (bit 0 is clear/unmasked)
    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);

    serial_print("PIT: initialized at ~");
    serial_print_hex_u64(freqHz);
    serial_println(" Hz");

    if (!interrupts_register_handler(0x00, pit_handle_irq0, NULL)) {
        serial_println("Failed to register handler for IRQ0");
    }
}

void timer_set_tick_handler(timer_tick_handler_t handler, void *priv) {
    g_tick_handler = handler;
    g_tick_handler_priv = priv;
}

uint64_t timer_get_ticks(void) {
    return g_ticks;
}

bool pit_handle_irq0(struct interrupt_frame *frame, void *priv) {
    g_ticks++;

    if (g_tick_handler != NULL) {
        return g_tick_handler(g_tick_handler_priv);
    }

    return true;
}
