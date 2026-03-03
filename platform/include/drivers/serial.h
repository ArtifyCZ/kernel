#pragma once

#include "early_console.h"
#include "stdint.h"

#define serial_write(byte) early_console_write(byte)

#define serial_print(message) early_console_print(message)

#define serial_println(message) early_console_println(message)

#define serial_print_hex_u64(value) early_console_print_hex_u64(value)
