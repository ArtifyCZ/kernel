#include "syscalls.h"
#include "syscalls_dispatch.h"

#include <stddef.h>
#include <stdint.h>

#include "boot.h"
#include "sys_exit.h"
#include "sys_write.h"
#include "drivers/serial.h"

static const syscall_handler g_syscall_handlers_table[] = {
    [SYS_EXIT] = sys_exit,
    [SYS_WRITE] = sys_write,
};

#define SYSCALLS_COUNT (sizeof(g_syscall_handlers_table) / sizeof(syscall_handler))

uint64_t syscalls_dispatch(struct syscall_frame *frame) {
    if (frame->num >= SYSCALLS_COUNT || g_syscall_handlers_table[frame->num] == NULL) {
        // @TODO: add more robust handling when syscall does not exist
        serial_println("=== KERNEL PANIC ===");
        serial_println("NON-EXISTENT SYSCALL TRIGGERRED");
        serial_print("Syscall number: ");
        serial_print_hex_u64(frame->num);
        hcf();
    }

    return g_syscall_handlers_table[frame->num](frame);
}
