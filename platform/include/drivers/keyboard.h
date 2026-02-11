#pragma once

#include <stdbool.h>
#include <stdint.h>

void keyboard_init(void);

bool keyboard_pop_scancode(uint8_t *out);

bool keyboard_get_char(char *out);
