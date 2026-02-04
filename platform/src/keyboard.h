//
// Created by artify on 2/4/26.
//

#ifndef KERNEL_2026_01_31_KEYBOARD_H
#define KERNEL_2026_01_31_KEYBOARD_H

#include "stdbool.h"
#include "stdint.h"

bool keyboard_read_char(uint8_t *out);

char kbd_translate_scancode(uint8_t sc);

void kbd_read_and_push(void);

bool kbd_scancode_push(uint8_t sc);

#endif //KERNEL_2026_01_31_KEYBOARD_H
