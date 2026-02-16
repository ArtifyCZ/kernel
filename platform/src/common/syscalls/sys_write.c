#include "sys_write.h"

#include "drivers/serial.h"
#include <stddef.h>

static uint64_t do_serial_write(const char *buf, const size_t count) {
    for (size_t i = 0; i < count; i++) {
        const uint8_t byte = buf[i];
        serial_write(byte);
    }

    return 0;
}

uint64_t sys_write(struct syscall_frame *frame) {
    int fd = (int) frame->a[0];
    uintptr_t user_buf = (uintptr_t) frame->a[1];
    size_t count = (size_t) frame->a[2];

    // 1. Basic Range Check: Is the buffer in User Space?
    // On x86_64, user addresses are usually < 0x00007FFFFFFFFFFF
    if (user_buf >= 0x800000000000ULL || (user_buf + count) >= 0x800000000000ULL) {
        return -1; // EFAULT: Bad Address
    }

    // 2. Resolve the File Descriptor
    if (fd == 1 || fd == 2) {
        // stdout or stderr
        return do_serial_write((char *) user_buf, count);
    }

    return -1; // EBADF: Bad File Descriptor}
}
