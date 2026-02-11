#include "../common/keyboard_priv.h"

#include "interrupts.h"
#include "io_wrapper.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "../../../arch/x86_64/ioapic.h"

#define KEYBOARD_COMMAND_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60

#define KEYBOARD_INTERRUPT_VECTOR 0x31

bool keyboard_interrupt_handler(struct interrupt_frame *frame, void *priv);

static bool keyboard_read_scancode(uint8_t *out);

void keyboard_arch_init(void) {
    interrupts_register_handler(KEYBOARD_INTERRUPT_VECTOR, keyboard_interrupt_handler, NULL);
    ioapic_set_entry(0x01, KEYBOARD_INTERRUPT_VECTOR);

    uint8_t sc;
    while (keyboard_read_scancode(&sc)) {
        // clear keyboard buffer
    }
}

static bool keyboard_read_scancode(uint8_t *out) {
    const uint8_t status = inb(KEYBOARD_COMMAND_PORT);
    if ((status & 0x01) == 0) {
        return false;
    }

    *out = inb(KEYBOARD_DATA_PORT);
    return true;
}

bool keyboard_interrupt_handler(struct interrupt_frame *frame, void *priv) {
    uint8_t sc;

    while (keyboard_read_scancode(&sc)) {
        keyboard_buffer_push(sc);
    }

    return true;
}
