#pragma once

#include <stdbool.h>
#include <stdint.h>

void keyboard_arch_init(void);

bool keyboard_buffer_push(uint8_t item);
