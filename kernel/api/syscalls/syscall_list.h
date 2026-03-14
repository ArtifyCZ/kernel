#pragma once

// All syscalls in a macro list
#define SYSCALLS_LIST(X) \
    /* Name         NAME        Num     Ret         Cnt   Args... */ \
    /* Exits the current task immediately */ \
    X(exit,         EXIT,       0x00,   void,       0) \
    /* Writes bytes to the specific file descriptor */ \
    X(write,        WRITE,      0x01,   void,       3,  int, fd, const void*, buf, size_t, count) \
    /* Blocks the current task till an interrupt with the provided IRQ is fired */ \
    X(irq_wait,     IRQ_WAIT,   0x04,   void,       1,  uint8_t, irq) \
    /* Unmasks interrupts of the specific IRQ, usually used before the IRQ_WAIT syscall */ \
    X(irq_unmask,   IRQ_UNMASK, 0x05,   void,       1,  uint8_t, irq) \
    /* Maps a new page-aligned MMIO device memory chunk into the address space to the provided physical address */ \
    X(mmap_dev,     MMAP_DEV,   0x06,   uintptr_t,  5,  uintptr_t, addr, uintptr_t, len, uintptr_t, phys, uint32_t, pr, uint32_t, fl) \
    /* Creates a new 'process' container and returns its handle. */ \
    /* This automatically creates a new address space and a new empty capabilities space. */ \
    X(proc_create,  PROC_CREATE,0x07,   uint64_t,   1,  uint64_t, fl) \
    /* Maps a memory chunk into an address space; handle 0 is 'self' */ \
    X(proc_mmap,    PROC_MMAP,  0x08,   uintptr_t,  5,  uint64_t, proc_handle, uint64_t, addr, uint64_t, len, uint32_t, pr, uint32_t, fl) \
    /* Changes protection flags of a mapped memory chunk */ \
    X(proc_mprot,   PROC_MPROT, 0x09,   void,       4,  uint64_t, proc_handle, uint64_t, addr, uint64_t, len, uint32_t, pr) \
    /* Unmaps a memory chunk from an address space */ \
    X(proc_munmap,  PROC_MUNMAP,0x0A,   void,       3,  uint64_t, proc_handle, uint64_t, addr, uint64_t, len) \
    /* Spawns a new task with a provided 'process' container (address space and capabilities space) */ \
    X(proc_spawn,   PROC_SPAWN, 0x0B,   uint64_t,   5,  uint64_t, proc_handle, uint64_t, fl, uintptr_t, sp, uintptr_t, ip, uint64_t, arg) \
    /* End of the list */

// Memory-mapping protection flags
#define SYS_PROT_READ       0x01
#define SYS_PROT_WRITE      0x02
#define SYS_PROT_EXEC       0x04

// Memory-mapping flags
#define SYS_MMAP_FL_MIRROR  0x01
