//
// Created by artify on 2/1/26.
//

#ifndef KERNEL_2026_01_31_INTERRUPTS_H
#define KERNEL_2026_01_31_INTERRUPTS_H

void idt_init(void);

__attribute__((noreturn))
void exception_handler(void);

#endif //KERNEL_2026_01_31_INTERRUPTS_H
