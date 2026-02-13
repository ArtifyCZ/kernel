.global arch_thread_entry

arch_thread_entry:
    // Arguments were loaded into x19-x21 by the context switch 'ldp' instructions
    // from the stack frame initialized in thread_setup.
    mov x0, x20    // fn
    mov x1, x21    // arg
    br x19         // jump to the trampoline (scheduler_wrapper)
