#include "scheduler.h"

#include "drivers/serial.h"

_Noreturn void sched_start(void) {
    serial_println("sched: starting");

#if defined (__x86_64__)
    __asm__ volatile ("int $0x30"); // @TODO: replace this by a proper thread yield syscall or something
#endif

    // Should never reach here on x86_64
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__)
        asm ("wfi");
#endif
    }
}
