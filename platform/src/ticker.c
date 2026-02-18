#include "ticker.h"

#include "scheduler.h"
#include "stddef.h"
#include "timer.h"
#include "drivers/serial.h"

bool ticker_tick_handler(struct interrupt_frame **frame, void *priv);

void ticker_init(void) {
    timer_set_tick_handler(ticker_tick_handler, NULL);
}

bool ticker_tick_handler(struct interrupt_frame **frame, void *priv) {
    (void) priv;

    *frame = (struct interrupt_frame *) sched_heartbeat((struct thread_ctx *) *frame);

    uint64_t ticks = timer_get_ticks();
    if (ticks % 100 == 0) {
        serial_print("Timer ticks: ");
        serial_print_hex_u64(ticks);
        serial_println("");
    }

    return true;
}
