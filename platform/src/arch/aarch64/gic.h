// Generic interrupt controller on aarch64

#pragma once

#include <stdint.h>

void gic_init(void);

void gic_enable_interrupt(uint32_t vector);

void gic_configure_interrupt(uint32_t vector, uint8_t priority);

uint32_t gic_acknowledge_interrupt(void);

void gic_end_of_interrupt(uint32_t id);
