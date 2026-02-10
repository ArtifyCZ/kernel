#include "drivers/serial.h"
#include <stdint.h>
#include <stddef.h>

#include "virtual_address_allocator.h"
#include "virtual_memory_manager.h"

// Private register offsets for the PL011
#define UART_DR    (0x00 / 4)
#define UART_FR    (0x18 / 4)
#define UART_IBRD  (0x24 / 4)
#define UART_FBRD  (0x28 / 4)
#define UART_LCRH  (0x2C / 4)
#define UART_CR    (0x30 / 4)

#define FR_TXFF    (1 << 5)

static volatile uint32_t *uart_base = NULL;

int serial_init(uintptr_t physical_base) {
    uintptr_t virtual_base = vaa_alloc_range(VMM_PAGE_SIZE);
    uart_base = (volatile uint32_t *)virtual_base;

    // We use VMM_FLAG_DEVICE to ensure the MMU treats this as MMIO (no caching/reordering).
    bool success = vmm_map_page(
        &g_kernel_context,
        virtual_base,
        physical_base,
        VMM_FLAG_PRESENT | VMM_FLAG_WRITE | VMM_FLAG_DEVICE
    );

    if (!success) {
        // If mapping fails, we can't initialize the serial port.
        // On a real kernel, you might hang or panic here.
        return -1;
    }

    // 3. Hardware Initialization (PL011 specific)
    uart_base[UART_CR] = 0;               // Disable UART
    uart_base[UART_LCRH] = (3 << 5);      // 8 bits, no parity, 1 stop bit (8N1)
    uart_base[UART_CR] = (1 << 0) | (1 << 8) | (1 << 9); // Enable UART, TX, RX

    return 0;
}

static int is_transmit_empty() {
    if (!uart_base) return 0;
    return !(uart_base[UART_FR] & FR_TXFF);
}

static void write_serial(char a) {
    if (!uart_base) return;

    if (a == '\n') {
        write_serial('\r');
    }

    while (!is_transmit_empty()) {
        __asm__ volatile("yield");
    }

    uart_base[UART_DR] = (uint32_t)a;
}

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