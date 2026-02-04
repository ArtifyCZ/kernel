//
// Created by Richard Tichý on 04.02.2026.
//

#ifndef KERNEL_PIT_H
#define KERNEL_PIT_H


#include <stdint.h>

void pit_init(uint32_t hz);
void pit_handle_irq0(void);

uint64_t pit_ticks(void);

#endif //KERNEL_PIT_H
