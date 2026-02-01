#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "cga_graphics.h"

// The CGA graphics device is memory-mapped. We're not going to go into a ton 
// of detail here, but the basic idea is:

// | 4 bit background color | 4 bit foreground color | 8 bit ascii character |
// 24 rows of text, with 80 columns per row
volatile uint16_t* cga_memory = (volatile uint16_t*)0xb8000;
const uint32_t cga_column_count = 80;
const uint32_t cga_row_count = 24;
const uint16_t cga_white_on_black_color_code = (15 << 8);

void cga_clear_screen(void) {
    // would be nice to use memset here, but remember, there is no libc available
    for (uint32_t i = 0; i < cga_row_count * cga_column_count; ++i) {
        cga_memory[i] = 0;
    }
}

void cga_print_string(const char* string, uint32_t column, uint32_t row) {
    // this allows us to better control where on screen text will appear
    const uint32_t initial_index = column + cga_column_count * row;

    for (uint32_t i = initial_index; *string != '\0'; ++string, ++i) {
        cga_memory[i] = *string | cga_white_on_black_color_code;
    }
}
