/* dsctbl.c GDT IDT分段及中断相关 */

#include "bootpack.h"


/////////////////////////////////////////////////////////////////////////////////////
//功能：
//参数：
//附件：
void init_gdtidt(void)
{
    /*在bootpack.h文件里面定义了分段结构体和中断结构体，这里实例化gdt和idt*/
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;//从0x00270000~0x0027ffff共8192个分段
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;//从0x0026f800~0x0026ffff共256个分段
    int i;

    /* GDT的初始化 */
    for (i = 0; i <= LIMIT_GDT / 8; i++) {
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    //，上限，地址，属性
    set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);//设定全局段
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);//设定bootpack512字节的段
    /* ;1) 这个函数用来指定的段上限和地址赋值给GDTR的48位寄存器，这是个很特殊的寄存器，并不能用MOV来直接赋值
    ;，唯一的方法就是指定一个内存地址，从指定的内存地址读取6字节（也就是48位），然后赋值给GDTR寄存器。完成这一任务的指令就是LGDT
    ;2) 该寄存器的低16位（即内存的最初的2个字节）是段上限，它等于“GDT的有效的字节数-1”，剩下的32位，代表GDT的开始地址
    ;3) 这里2个参数是ESP+4和ESP+8里存放，而我们要的是6字节形式的，所以要转换为我们想要的形式~ */
    load_gdtr(LIMIT_GDT, ADR_GDT);

    /* IDT的初始化 */
    for (i = 0; i <= LIMIT_IDT / 8; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(LIMIT_IDT, ADR_IDT);

    /* IDT的設定 */
    set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
    /*这里asm_inthadler21注册在idt的第0x21号，这样如果发生了中断，CPU就会自动调用asm_inthandler21.
    这里的2*8表示asm_inthandler21属于哪一个段，即段号2，乘以8是因为低3位有别的意思，这低3位必须为0
    这里段号为2的段在初始化的地方我们设置为bootpack全局段
    最后的AR_INTGATE32将IDT的属性，设定为0x008e,表示用于中断处理的有效性
    */
    return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff) {
        ar |= 0x8000; /* G_bit = 1 */
        limit /= 0x1000;
    }
    sd->limit_low    = limit & 0xffff;
    sd->base_low     = base & 0xffff;
    sd->base_mid     = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high    = (base >> 24) & 0xff;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    gd->offset_low   = offset & 0xffff;
    gd->selector     = selector;
    gd->dw_count     = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high  = (offset >> 16) & 0xffff;
    return;
}

