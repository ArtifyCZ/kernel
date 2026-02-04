//
// Created by artify on 2/4/26.
// PIC - Programmable interrupt controller
//

#ifndef KERNEL_2026_01_31_PIC_H
#define KERNEL_2026_01_31_PIC_H

#include <stdint.h>

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

void pic_send_eoi(uint8_t irq);

void pic_remap(uint8_t offset1, uint8_t offset2);

void pic_disable(void);

#endif //KERNEL_2026_01_31_PIC_H
