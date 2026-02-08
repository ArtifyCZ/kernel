#pragma once

#include <stdint.h>

void init_mair(void);

void dirty_bootstrap_map_uart(uintptr_t vaddr, uintptr_t paddr);
