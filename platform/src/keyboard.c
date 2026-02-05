//
// Created by artify on 2/4/26.
//

#include "keyboard.h"

#include "io_wrapper.h"
#include "stdbool.h"
#include "stdint.h"
#include "serial.h"

#define KEYBOARD_COMMAND_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60

#define KBD_BUF_SIZE 128

static volatile uint8_t kbd_buf[KBD_BUF_SIZE];
static volatile uint32_t kbd_head = 0; // written by producer (IRQ)
static volatile uint32_t kbd_tail = 0; // written by consumer (main loop)

static inline uint32_t kbd_next(uint32_t x) {
    return (x + 1) % KBD_BUF_SIZE;
}

bool kbd_scancode_push(uint8_t sc) {
    uint32_t head = kbd_head;
    uint32_t next = kbd_next(head);
    if (next == kbd_tail) {
        // overflow: drop
        return false;
    }
    kbd_buf[head] = sc;
    kbd_head = next;
    return true;
}

bool kbd_scancode_pop(uint8_t *out) {
    uint32_t tail = kbd_tail;
    if (tail == kbd_head) return false;
    *out = kbd_buf[tail];
    kbd_tail = kbd_next(tail);
    return true;
}

bool keyboard_read_scancode(uint8_t *out) {
    const uint8_t status = inb(KEYBOARD_COMMAND_PORT);
    if ((status & 0x01) == 0) {
        return false;
    }

    *out = inb(KEYBOARD_DATA_PORT);
    return true;
}

void kbd_read_and_push(void) {
    uint8_t code;
    while (keyboard_read_scancode(&code)) {
        if (!kbd_scancode_push(code)) {
            return;
        }
    }
}

static bool shift_down = false;

static const char map_no_shift[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm',
    [0x39] = ' ',
    [0x1C] = '\n',
    [0x0E] = '\b',
};

static const char map_shift[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%', [0x07] = '^', [0x08] = '&', [0x09] = '*',
    [0x0A] = '(', [0x0B] = ')',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I',
    [0x18] = 'O', [0x19] = 'P',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K',
    [0x26] = 'L',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N', [0x32] = 'M',
    [0x39] = ' ',
    [0x1C] = '\n',
    [0x0E] = '\b',
};

static inline bool is_break(uint8_t sc) { return (sc & 0x80) != 0; }

static inline uint8_t make_code(uint8_t sc) { return sc & 0x7F; }

char kbd_translate_scancode(uint8_t sc) {
    // Shift make codes in set 1: 0x2A (LShift), 0x36 (RShift)
    if (!is_break(sc) && (sc == 0x2A || sc == 0x36)) {
        shift_down = true;
        return 0;
    }
    if (is_break(sc) && (make_code(sc) == 0x2A || make_code(sc) == 0x36)) {
        shift_down = false;
        return 0;
    }

    if (is_break(sc)) return 0; // ignore key releases for now

    uint8_t mc = make_code(sc);
    char c = shift_down ? map_shift[mc] : map_no_shift[mc];
    return c; // 0 if unmapped
}

bool keyboard_read_char(uint8_t *out) {
    if (out == (uint8_t *) 0x64) {
        serial_println("Should not really happen!!!");
    }
    return kbd_scancode_pop(out);
}
