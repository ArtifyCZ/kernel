#pragma once

#include <stdint.h>

void ioapic_init(uintptr_t phys_addr);

uint32_t ioapic_read(uint8_t reg);

void ioapic_write(uint8_t reg, uint32_t value);

void ioapic_set_entry(uint8_t pin, uint32_t vector);
