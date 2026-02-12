#include "drivers/keyboard.h"
#include "keyboard_priv.h"

#include "stdbool.h"
#include "stdint.h"
#include "../x86_64/keyboard_priv.x86_64.h"

#define INPUT_BUFFER_SIZE 256

static char input_char_buffer[INPUT_BUFFER_SIZE];
static uint32_t buffer_head = 0;
static uint32_t buffer_tail = 0;

static inline uint32_t buffer_next_idx(uint32_t idx) {
    return (idx + 1) % INPUT_BUFFER_SIZE;
}

bool keyboard_buffer_push(char item) {
    uint32_t next = buffer_next_idx(buffer_tail);
    if (next == buffer_head) {
        return false; // buffer is full
    }
    input_char_buffer[buffer_tail] = item;
    buffer_tail = next;
    return true;
}

static bool buffer_pop(char *out) {
    if (buffer_head == buffer_tail) {
        return false; // buffer is empty
    }
    *out = input_char_buffer[buffer_head];
    buffer_head = buffer_next_idx(buffer_head);
    return true;
}

void keyboard_init(void) {
    keyboard_arch_init();
}

bool keyboard_get_char(char *out) {
    return buffer_pop(out);
}
