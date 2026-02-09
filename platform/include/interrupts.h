#pragma once

#include <stdint.h>
#include <stdbool.h>

// Represents the CPU state saved during an interrupt.
// This will be defined per-architecture in a separate header.
struct interrupt_frame;

// Callback signature for an interrupt handler.
// Returns true if the interrupt was handled.
typedef bool (*irq_handler_t)(struct interrupt_frame *frame, void *priv);

// High-level interrupt types
typedef enum {
    IRQ_TYPE_EDGE_RISING,
    IRQ_TYPE_EDGE_FALLING,
    IRQ_TYPE_LEVEL_HIGH,
    IRQ_TYPE_LEVEL_LOW,
} irq_type_t;

/**
 * Global interrupt management
 */

// Initialize the architecture-specific interrupt controller (GIC or APIC)
void interrupts_init(void);

// Enable/Disable interrupts globally on the current CPU
void interrupts_enable(void);
void interrupts_disable(void);

/**
 * IRQ Routing
 */

// Registers a handler for a specific IRQ line
// irq: The hardware IRQ number
// handler: The function to call
// priv: Private data passed back to the handler
bool interrupts_register_handler(uint32_t irq, irq_handler_t handler, void *priv);

// Unregister a handler
bool interrupts_unregister_handler(uint32_t irq);

// Configure an IRQ (trigger type, priority, etc.)
void interrupts_configure_irq(uint32_t irq, irq_type_t type, uint8_t priority);
