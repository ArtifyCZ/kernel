#include "main.h"
#include "stddef.h"
#include "stdint.h"
#include "boot.h"
#include "serial.h"

__attribute__((unused)) void main(void) {
    serial_println("Hello world!");
    serial_println("How are you btw?");

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    for (size_t i = 0; i < 100; i++) {
        volatile uint32_t *fb_ptr = framebuffer->address;
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xff3322;
    }
}
