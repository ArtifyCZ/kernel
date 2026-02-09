#include "ticker.h"

#include "stddef.h"
#include "timer.h"
#include "drivers/serial.h"

bool ticker_tick_handler(void *priv);

void ticker_init(void) {
    timer_set_tick_handler(ticker_tick_handler, NULL);
}

bool ticker_tick_handler(void *priv) {
    (void) priv;

    uint64_t ticks = timer_get_ticks();
    if (ticks % 100 == 0) {
        serial_print("Timer ticks: ");
        serial_print_hex_u64(ticks);
        serial_println("");
    }

    return true;
}
