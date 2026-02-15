#include "sys_exit.h"

#include "boot.h"
#include "syscalls_dispatch.h"
#include "drivers/serial.h"

uint64_t sys_exit(struct syscall_frame *frame) {
    // @TODO: implement
    serial_println("=== EXIT SYSCALL ===");
    hcf();
}
