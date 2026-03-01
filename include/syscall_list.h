#pragma once

// All syscalls in a macro list
#define SYSCALLS_LIST(X) \
    /* Name     NAME    Num     Ret         Cnt   Args... */ \
    X(exit,     EXIT,   0x00,   void,       0) \
    X(write,    WRITE,  0x01,   void,       3,    int, fd, const void*, buf, size_t, count) \
    X(clone,    CLONE,  0x02,   uint64_t,   3,    uint64_t, flags, const void*, sp, const void*, ip) \
    X(mmap,     MMAP,   0x03,   uintptr_t,  4,    uint64_t, addr, uint64_t, len, uint32_t, pr, uint32_t, fl) \
    /* End of the list */
