#include "drivers/serial.h"
#include <stdint.h>
#include <stddef.h>

#include "dirty_vmm_priv.h"

// Private register offsets for the PL011
// The division by 4 is to index correctly, as C multiplies by the size of the variable pointed on
#define UART_DR    (0x00 / 4)     // Data Register
#define UART_FR    (0x18 / 4)     // Flag Register
#define UART_IBRD  (0x24 / 4)     // Integer Baud Rate
#define UART_FBRD  (0x28 / 4)     // Fractional Baud Rate
#define UART_LCRH  (0x2C / 4)     // Line Control Register
#define UART_CR    (0x30 / 4)     // Control Register

// Flag register bits
#define FR_TXFF    (1 << 5) // Transmit FIFO full
#define FR_RXFE    (1 << 4) // Receive FIFO empty

static volatile uint32_t *uart_base = NULL;

int serial_init(uintptr_t base) {
    uart_base = (volatile uint32_t *)(base + 0xFFFFC00000000000); // adding heap base address

    init_mair();
    dirty_bootstrap_map_uart((uintptr_t) uart_base, base);

    // Test read before write
    uint32_t test = uart_base[UART_FR];
    (void)test;

    // For early boot, Limine/Firmware usually initializes the UART.
    // If you need a hard reset, you'd disable UART, set baud, then re-enable.
    // Minimal "ensure enabled" sequence:
    uart_base[UART_CR] = 0;               // Disable UART
    uart_base[UART_LCRH] = (3 << 5);      // 8 bits, no parity, 1 stop bit (8N1)
    uart_base[UART_CR] = (1 << 0) | (1 << 8) | (1 << 9); // Enable UART, TX, RX

    return 0;
}

static int is_transmit_empty() {
    if (!uart_base) return 0;
    // Check if the Transmit FIFO is NOT full
    return !(uart_base[UART_FR] & FR_TXFF);
}

static void write_serial(char a) {
    if (!uart_base) return;

    // Carriage return handling: if we send \n, many terminals also need \r
    if (a == '\n') {
        write_serial('\r');
    }

    // Wait until there is space in the FIFO
    while (!is_transmit_empty()) {
        __asm__ volatile("yield"); // Hint to CPU we are in a spin-loop
    }

    uart_base[UART_DR] = (uint32_t)a;
}

// --- Your existing print logic remains the same ---

void serial_print(const char *message) {
    if (!message) return;
    for (size_t i = 0; message[i] != '\0'; i++) {
        write_serial(message[i]);
    }
}

void serial_println(const char *message) {
    serial_print(message);
    write_serial('\n');
}

void serial_print_hex_u64(uint64_t value) {
    static const char *hex = "0123456789abcdef";
    serial_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nib = (value >> i) & 0xF;
        write_serial(hex[nib]);
    }
}
