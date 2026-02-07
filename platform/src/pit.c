//
// Created by Richard Tichý on 04.02.2026.
//

#include "pit.h"

#include "io_wrapper.h"
#include "drivers/serial.h"
#include "scheduler.h"

#define PIT_CH0_DATA  0x40
#define PIT_CMD       0x43

static volatile uint64_t g_ticks = 0;

uint64_t pit_ticks(void) {
    return g_ticks;
}

void pit_init(uint32_t hz) {
    if (hz == 0) hz = 100;

    // PIT input clock is ~1_193_182 Hz
    const uint32_t pit_base = 1193182u;
    uint16_t divisor = (uint16_t) (pit_base / hz);
    if (divisor == 0) divisor = 1;

    // Command: channel 0, access lobyte/hibyte, mode 3 (square wave), binary
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0_DATA, (uint8_t) (divisor & 0xFF));
    outb(PIT_CH0_DATA, (uint8_t) ((divisor >> 8) & 0xFF));

    serial_print("PIT: initialized at ~");
    serial_print_hex_u64(hz);
    serial_println(" Hz");
}

void pit_handle_irq0(void) {
    g_ticks++;

    // Ask scheduler to reschedule soon (cooperative).
    sched_request_reschedule();

    // Optional: very low-rate debug
    // if ((g_ticks % 100) == 0) serial_println("tick");
}
