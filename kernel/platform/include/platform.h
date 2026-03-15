#pragma once

#include "limine.h"

struct platform_config {
    struct limine_framebuffer *framebuffer;
    void *rsdp_address;
};

void platform_init(const struct platform_config *config);
