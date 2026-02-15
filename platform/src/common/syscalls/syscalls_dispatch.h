#pragma once

#include <stdint.h>
#include "syscalls.h"

#define SYS_EXIT 0x00
#define SYS_WRITE 0x01

typedef uint64_t (*syscall_handler)(struct syscall_frame *frame);

#define SYSCALL_HANDLER(name) uint64_t name(struct syscall_frame *frame)
