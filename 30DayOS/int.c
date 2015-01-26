/* the part of interrupt */

#include "bootpack.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////////////
//功能：中断初始化函数
//参数：无
void init_pic(void)
/* PIC(programmable interrupt controller) initialize */
{
    io_out8(PIC0_IMR,  0xff  ); /* disabled all interrupts */
    io_out8(PIC1_IMR,  0xff  ); /* disabled all interrupts */

    io_out8(PIC0_ICW1, 0x11  ); /* edge trigger mode */
    io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7 are received by INT20-27 */
    io_out8(PIC0_ICW3, 1 << 2); /* PIC1 is connected by IRQ2 */
    io_out8(PIC0_ICW4, 0x01  ); /* unbuffered mode */

    io_out8(PIC1_ICW1, 0x11  ); /* edge trigger mode */
    io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15 are received by INT28-2f */
    io_out8(PIC1_ICW3, 2     ); /* PIC1 is connnected by IRQ2 */
    io_out8(PIC1_ICW4, 0x01  ); /* unbuffered mode */

    io_out8(PIC0_IMR,  0xfb  ); /* 11111011 disable all interrupts except PIC1 */
    io_out8(PIC1_IMR,  0xff  ); /* 11111111 disable all interrupts */

    return;
}

void inthandler27(int *esp)                         
{
    io_out8(PIC0_OCW2, 0x67); /* IRQ-07受付完了をPICに通知 */
    return;
}

