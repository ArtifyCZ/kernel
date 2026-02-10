#include "interrupts.h"
#include "io_wrapper.h"
#include "drivers/serial.h"
#include "stddef.h"
#include "timer.h"

#include "virtual_address_allocator.h"
#include "virtual_memory_manager.h"

#define PIT_FREQ 1193182
#define TARGET_MS 10

// LAPIC Register Offsets (Divided by 4 for uint32_t pointer arithmetic)
#define LAPIC_EOI      (0x0B0 / 4)
#define LAPIC_SVR      (0x0F0 / 4)
#define LAPIC_LVT_TMR  (0x320 / 4)
#define LAPIC_TICRET   (0x380 / 4) // Initial Count
#define LAPIC_TCCR     (0x390 / 4) // Current Count
#define LAPIC_TDCR     (0x3E0 / 4) // Divide Configuration

#define PIT_CH0_DATA  0x40
#define PIT_CMD       0x43

// We'll use a vector that doesn't clash with legacy exceptions (0x00-0x2F)
// 0x30 is a safe choice for user-defined interrupts.
#define LAPIC_TIMER_VECTOR 0x30

#define LAPIC_TIMER_DIVISOR 0x3

bool lapic_timer_handler(struct interrupt_frame *frame, void *priv);

static volatile uint32_t *g_lapic_ptr = NULL;
static volatile uint32_t g_lapic_ticks_per_ms = 0;
static volatile uint64_t g_ticks = 0;
static volatile timer_tick_handler_t g_tick_handler = NULL;
static volatile void *g_tick_handler_priv = NULL;

static void lapic_calibrate_timer(void) {
    // Software-enable LAPIC (Set bit 8 of Spurious Vector Register)
    g_lapic_ptr[LAPIC_SVR] |= 0x100;

    // Set divider to 16
    g_lapic_ptr[LAPIC_TDCR] = LAPIC_TIMER_DIVISOR;

    // Prepare PIT for 10ms (Mode 0: Interrupt on Terminal Count)
    uint16_t pit_reload_value = PIT_FREQ / 100;
    outb(PIT_CMD, 0x30);
    outb(PIT_CH0_DATA, (uint8_t) (pit_reload_value & 0xFF));
    outb(PIT_CH0_DATA, (uint8_t) ((pit_reload_value >> 8) & 0xFF));

    // Start the LAPIC timer at max
    g_lapic_ptr[LAPIC_TICRET] = 0xFFFFFFFF;

    // Poll PIT until it wraps/hits zero
    while (1) {
        outb(PIT_CMD, 0x00); // Latch
        uint8_t low = inb(PIT_CH0_DATA);
        uint8_t high = inb(PIT_CH0_DATA);
        if ((low | (high << 8)) == 0) break;
    }

    // 6. Calculate frequency
    uint32_t ticks_in_10ms = 0xFFFFFFFF - g_lapic_ptr[LAPIC_TCCR];
    g_lapic_ptr[LAPIC_TICRET] = 0; // Stop

    g_lapic_ticks_per_ms = ticks_in_10ms / TARGET_MS;
}

void timer_init(uint32_t freqHz) {
    if (freqHz == 0) freqHz = 100;

    // Map LAPIC if not already mapped
    if (g_lapic_ptr == NULL) {
        uintptr_t lapic_virt_addr = vaa_alloc_range(VMM_PAGE_SIZE);
        g_lapic_ptr = (volatile uint32_t *) lapic_virt_addr;
        vmm_map_page(
            &g_kernel_context,
            lapic_virt_addr,
            0xFEE00000,
            VMM_FLAG_PRESENT | VMM_FLAG_WRITE | VMM_FLAG_DEVICE
        );
    }

    // Mask legacy PIC entirely (we are done with it)
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    lapic_calibrate_timer();

    // Register the new LAPIC handler
    interrupts_register_handler(LAPIC_TIMER_VECTOR, lapic_timer_handler, NULL);

    // Calculate initial count: (ticks_per_ms * 1000) / freqHz
    uint32_t initial_count = (g_lapic_ticks_per_ms * 1000) / freqHz;

    // Configure LVT Timer: Periodic Mode (bit 17) + Vector
    g_lapic_ptr[LAPIC_LVT_TMR] = LAPIC_TIMER_VECTOR | (1 << 17);
    g_lapic_ptr[LAPIC_TDCR] = LAPIC_TIMER_DIVISOR;
    g_lapic_ptr[LAPIC_TICRET] = initial_count;

    serial_print("LAPIC Timer: initialized at ");
    serial_print_hex_u64(freqHz);
    serial_println(" Hz");
}

void timer_set_tick_handler(timer_tick_handler_t handler, void *priv) {
    g_tick_handler = handler;
    g_tick_handler_priv = priv;
}

uint64_t timer_get_ticks(void) {
    return g_ticks;
}

bool lapic_timer_handler(struct interrupt_frame *frame, void *priv) {
    g_ticks++;

    bool ack = true;
    if (g_tick_handler != NULL) {
        ack = g_tick_handler((void *) g_tick_handler_priv);
    }

    // Signal End of Interrupt to LAPIC
    g_lapic_ptr[LAPIC_EOI] = 0;

    return ack;
}
