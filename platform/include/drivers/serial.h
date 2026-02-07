#pragma once

#include "stdint.h"

/**
 * @return Successful if zero, otherwise the initialization failed
 */
int serial_init(uintptr_t base);

void serial_print(const char *message);

void serial_println(const char *message);

void serial_print_hex_u64(uint64_t value);
