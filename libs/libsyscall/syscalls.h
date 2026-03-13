#pragma once

#include "kernel/api/syscalls/syscall_list.h"
#include "kernel/api/syscalls/syscall_errs.h"
#include <stdint.h>
#include <stddef.h>

struct syscall_result {
    enum syscall_err err_code;
    uint64_t value;
};

typedef struct syscall_result sys_result_t;

/* --- Architectural Abstraction --- */
// Expecting the platform to provide:
// sys_result_t __arch_syscall(uint64_t num, uint64_t a1, uint64_t a2, ...);

/* --- Internal Helpers --- */
#define _SS_LIST0()              void
#define _SS_LIST1(t1, n1)        t1 n1
#define _SS_LIST2(t1, n1, ...)   t1 n1, _SS_LIST1(__VA_ARGS__)
#define _SS_LIST3(t1, n1, ...)   t1 n1, _SS_LIST2(__VA_ARGS__)
#define _SS_LIST4(t1, n1, ...)   t1 n1, _SS_LIST3(__VA_ARGS__)
#define _SS_LIST5(t1, n1, ...)   t1 n1, _SS_LIST4(__VA_ARGS__)

#define _SS_VAL0()               0
#define _SS_VAL1(t1, n1)         (uint64_t)(n1)
#define _SS_VAL2(t1, n1, ...)    (uint64_t)(n1), _SS_VAL1(__VA_ARGS__)
#define _SS_VAL3(t1, n1, ...)    (uint64_t)(n1), _SS_VAL2(__VA_ARGS__)
#define _SS_VAL4(t1, n1, ...)    (uint64_t)(n1), _SS_VAL3(__VA_ARGS__)
#define _SS_VAL5(t1, n1, ...)    (uint64_t)(n1), _SS_VAL4(__VA_ARGS__)

/* --- The Dual-Register Wrapper Generator --- */

// Logic: Every syscall returns sys_result_t.
// If userspace wants the raw value, they access .value
#define SYSCALL_USER_WRAPPER(name, NAME, num, ret, count, ...) \
    _SS_USER_WRAPPER_##ret(name, NAME, num, ret, count, __VA_ARGS__) \
    /* */

#define _SS_USER_WRAPPER_void(name, NAME, num, ret, count, ...) \
    static inline sys_err_t sys_##name(_SS_LIST##count(__VA_ARGS__)) { \
        return __arch_syscall(num, _SS_VAL##count(__VA_ARGS__)).err_code; \
    } \
    /* */

#define _SS_USER_WRAPPER_uint64_t(name, NAME, num, ret, count, ...) \
    static inline sys_err_t sys_##name(_SS_LIST##count(__VA_ARGS__), ret *out) { \
        sys_result_t result = __arch_syscall(num, _SS_VAL##count(__VA_ARGS__)); \
        if (result.err_code == SYS_SUCCESS && out != NULL) *out = result.value; \
        return result.err_code; \
    } \
    /* */

#define _SS_USER_WRAPPER_uintptr_t(name, NAME, num, ret, count, ...) \
    static inline sys_err_t sys_##name(_SS_LIST##count(__VA_ARGS__), ret *out) { \
        sys_result_t result = __arch_syscall(num, _SS_VAL##count(__VA_ARGS__)); \
        if (result.err_code == SYS_SUCCESS && out != NULL) *out = result.value; \
        return result.err_code; \
    } \
    /* */

struct syscall_args {
    uint64_t num;
    uint64_t a[5];
};

static inline sys_result_t syscall_raw(struct syscall_args args) {
    sys_err_t err_code;
    uint64_t ret;
#if defined (__x86_64__)
    // Tie C variables to specific registers
    register uint64_t rax __asm__("rax") = args.num;
    register uint64_t rdi __asm__("rdi") = args.a[0];
    register uint64_t rsi __asm__("rsi") = args.a[1];
    register uint64_t rdx __asm__("rdx") = args.a[2];
    register uint64_t r10 __asm__("r10") = args.a[3];
    register uint64_t r8 __asm__("r8") = args.a[4];

    __asm__ volatile (
        "syscall"
        : "=a"(ret), "=d"(err_code)
        : "r"(rax), "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory" // Clobbers
    );
#elif defined (__aarch64__)
    // x8 is the syscall number
    register uint64_t x8 __asm__("x8") = args.num;

    // x0-x4 are the arguments
    register uint64_t x0 __asm__("x0") = args.a[0];
    register uint64_t x1 __asm__("x1") = args.a[1];
    register uint64_t x2 __asm__("x2") = args.a[2];
    register uint64_t x3 __asm__("x3") = args.a[3];
    register uint64_t x4 __asm__("x4") = args.a[4];

    __asm__ volatile (
        "svc #0"
        : "+r"(x0), "+r"(x1)
        : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4)
        : "memory" // Barrier to prevent compiler reordering
    );
    ret = x0;
    err_code = x1;
#else
#error "NOT IMPLEMENTED YET"
#endif
    sys_result_t result = { .err_code = err_code, .value = ret };
    return result;
}

#define __arch_syscall(number, ...) ({ \
    struct syscall_args _args = { .num = (number), .a = { __VA_ARGS__ } }; \
    syscall_raw(_args); \
})

SYSCALLS_LIST(SYSCALL_USER_WRAPPER);

const char *sys_err_get_message(sys_err_t err_code);
