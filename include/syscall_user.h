#pragma once

#include "syscall_list.h"

/* --- Internal Helpers (prefixed with _SS_) --- */

// Generate function parameter list: "type1 name1, type2 name2"
#define _SS_LIST0()              void
#define _SS_LIST1(t1, n1)        t1 n1
#define _SS_LIST2(t1, n1, ...)   t1 n1, _SS_LIST1(__VA_ARGS__)
#define _SS_LIST3(t1, n1, ...)   t1 n1, _SS_LIST2(__VA_ARGS__)
#define _SS_LIST4(t1, n1, ...)   t1 n1, _SS_LIST3(__VA_ARGS__)
#define _SS_LIST5(t1, n1, ...)   t1 n1, _SS_LIST4(__VA_ARGS__)
#define _SS_LIST6(t1, n1, ...)   t1 n1, _SS_LIST5(__VA_ARGS__)

// Generate values for syscall: "(uint64_t)n1, (uint64_t)n2"
#define _SS_VAL0()               0
#define _SS_VAL1(t1, n1)         (uint64_t)(n1)
#define _SS_VAL2(t1, n1, ...)    (uint64_t)(n1), _SS_VAL1(__VA_ARGS__)
#define _SS_VAL3(t1, n1, ...)    (uint64_t)(n1), _SS_VAL2(__VA_ARGS__)
#define _SS_VAL4(t1, n1, ...)    (uint64_t)(n1), _SS_VAL3(__VA_ARGS__)
#define _SS_VAL5(t1, n1, ...)    (uint64_t)(n1), _SS_VAL4(__VA_ARGS__)
#define _SS_VAL6(t1, n1, ...)    (uint64_t)(n1), _SS_VAL5(__VA_ARGS__)

/* --- The Wrapper Generator --- */

#define __SS_INLINE_void(name, NAME, num, ret, count, ...) \
    static inline void sys_##name(_SS_LIST##count(__VA_ARGS__)) { \
        __DO_SYSCALL(num, _SS_VAL##count(__VA_ARGS__)); \
    } \
    /* End of inline */
#define __SS_INLINE_ret(name, NAME, num, ret, count, ...) \
    static inline ret sys_##name(_SS_LIST##count(__VA_ARGS__)) { \
        return (ret)__DO_SYSCALL(num, _SS_VAL##count(__VA_ARGS__)); \
    } \
    /* End of inline */
#define __SS_INLINE_uint64_t(...) __SS_INLINE_ret(__VA_ARGS__)

#define SYSCALL_USER_WRAPPER(name, NAME, num, ret, count, ...) \
    __SS_INLINE_##ret(name, NAME, num, ret, count, __VA_ARGS__)

// Apply to generate all user-side inlines
SYSCALLS_LIST(SYSCALL_USER_WRAPPER);
