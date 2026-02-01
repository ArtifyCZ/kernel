/* cga_graphics.h - CGA graphics header file. */

#ifndef CGA_GRAPHICS_HEADER
#define CGA_GRAPHICS_HEADER 1


void cga_clear_screen(void);

void cga_print_string(const char* string, uint32_t column, uint32_t row);

#endif
