/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
/* Initialized to 0xFF (all off) */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask  = 0xFF;  /* IRQs 8-15 */

/* 
 * i8259_init
 *   DESCRIPTION: initialize the PIC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 1. initialize the master pic and slave pic
 *                 2. initialize the all the masks
 */
void i8259_init(void) {
    /* mask out all interrupts */
    outb(0xFF, MASTER_8259_PORT + 1);
    outb(0xFF, SLAVE_8259_PORT + 1);

    /* initialize the master pic 
     * ICW1: init; ICW2: vector mapping
     * ICW3: slave/master mask; ICW4: eoi
     */
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW2_MASTER, MASTER_8259_PORT + 1);
    outb(ICW3_MASTER, MASTER_8259_PORT + 1);
    outb(ICW4, MASTER_8259_PORT + 1);

    /* initialize the slave pic */
    outb(ICW1, SLAVE_8259_PORT);
    outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);
    outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);
    outb(ICW4, SLAVE_8259_PORT + 1);

    outb(master_mask, MASTER_8259_PORT + 1);
    outb(slave_mask, SLAVE_8259_PORT + 1);
}

/* 
 * enable_irq
 *   DESCRIPTION: enable an interrupt port on PIC
 *   INPUTS: irq_num - irq number for the device
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: two cases:
 *                 1. on master pic: change master mask
 *                 2. on slave pic: change both master and slave masks
 */
void enable_irq(uint32_t irq_num) {
    /* port 0-7: enable irq on the master port */
    if (irq_num <= 7) {
        master_mask &= ~(0x01 << irq_num);
        outb(master_mask, MASTER_8259_PORT + 1);
    }

    /* port 8-15: enable irq on the slave port */
    else if (irq_num >= 8 && irq_num <= 15) {
        master_mask &= ~(0x01 << 2);    // port 2 connect to slave PIC
        slave_mask &= ~(0x01 << (irq_num - 8));
        outb(master_mask, MASTER_8259_PORT + 1);
        outb(slave_mask, SLAVE_8259_PORT + 1);
    }
}

/* 
 * disable_irq
 *   DESCRIPTION: disable an interrupt port on PIC
 *   INPUTS: irq_num - irq number for the device
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: two cases:
 *                 1. on master pic: change master mask
 *                 2. on slave pic: change both master and slave masks
 */
void disable_irq(uint32_t irq_num) {
    /* port 0-7: disable irq on the master port */
    if (irq_num <= 7) {
        master_mask |= (0x01 << irq_num);
        outb(master_mask, MASTER_8259_PORT + 1);
    }

    /* port 8-15: disable irq on the slave port */
    else if (irq_num >= 8 && irq_num <= 15) {
        slave_mask |= (0x01 << (irq_num - 8));
        outb(slave_mask, SLAVE_8259_PORT + 1);
    }
}

/* 
 * send_eoi
 *   DESCRIPTION: send an eoi signal to the PIC
 *   INPUTS: irq_num - irq number for the device
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: two cases:
 *                 1. on master pic: send to master port
 *                 2. on slave pic: send to both master and slave ports
 */
void send_eoi(uint32_t irq_num) {
    /* port 0-7: send EOI to the master port */
    if (irq_num <= 7)
        outb((EOI | (uint8_t)irq_num), MASTER_8259_PORT);

    /* port 8-15: send EOI to the slave port */
    if (irq_num >= 8 && irq_num <= 15) {
        /* 0x02: pin 2 is connected to slave */
        outb((EOI | 0x02), MASTER_8259_PORT);
        outb((EOI | (uint8_t)(irq_num - 8)), SLAVE_8259_PORT);
    }
}
