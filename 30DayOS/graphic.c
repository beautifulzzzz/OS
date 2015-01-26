/* 操作系统函数集合 */

#include "bootpack.h"

// -----------------------------------------------------------------------------------
//调色板相关
//-----------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////
//功能：初始化调色板，建立静态16个颜色映射，然后调用set_palette进行设置
//参数：无
//附件：要调用set_palette函数
void init_palette(void)
{
     static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,    /*  0:黑 */
        0xff, 0x00, 0x00,    /*  1:亮红 */
        0x00, 0xff, 0x00,    /*  2:亮绿 */
        0xff, 0xff, 0x00,    /*  3:亮黄 */
        0x00, 0x00, 0xff,    /*  4:亮蓝 */
        0xff, 0x00, 0xff,    /*  5:亮紫 */
        0x00, 0xff, 0xff,    /*  6:浅亮蓝 */
        0xff, 0xff, 0xff,    /*  7:白 */
        0xc6, 0xc6, 0xc6,    /*  8:亮灰 */
        0x84, 0x00, 0x00,    /*  9:暗红 */
        0x00, 0x84, 0x00,    /* 10:暗绿 */
        0x84, 0x84, 0x00,    /* 11:暗黄 */
        0x00, 0x00, 0x84,    /* 12:暗青 */
        0x84, 0x00, 0x84,    /* 13:暗紫 */
        0x00, 0x84, 0x84,    /* 14:浅暗蓝 */
        0x84, 0x84, 0x84    /* 15:暗灰 */
    };
    set_palette(0, 15, table_rgb);
    return;
    /*C语言中static char只能用于数据，就像汇编中的DB指令 */
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：初始化设置调色板
//参数：开始标号，结束标号，16*3的颜色表
//附加：关闭中断，进行设置（要知道具体设置要求），恢复中断
void set_palette(int start, int end, unsigned char *rgb)
{
    int i, eflags;
    eflags = io_load_eflags();    /* 记录中断许可标志 */
    io_cli();                     /* 将中断许可标志置0，禁止中断 */
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);    /* 恢复原中断 */
    return;
}

//-----------------------------------------------------------------------------------
//显示主界面相关
//-----------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////
//功能：填充VRAM实现画矩形操作
//参数：VRAM初址，x轴像素，颜色标号，后面4个是位置矩形
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++)
            vram[y * xsize + x] = c;
    }
    return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：初始化屏幕，画矩形，形成最初的窗口界面
//参数：VRAM初址，屏幕宽和长
//附加：要调用boxfill8函数
void init_screen8(char *vram, int x, int y)
{
    boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
    boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
    boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
    boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

    boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
    boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
    boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
    boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
    boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
    boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

    boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
    boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
    boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
    boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
    return;
}

//-----------------------------------------------------------------------------------
//字符显示相关
//-----------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////
//功能：在指定位置显示一个字符
//参数：VRAM初址，窗口宽，待显示的位置，颜色，显示文字的初址（这里采用16个char表示一个字符）
void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
    int i;
    char *p, d /* data */;
    for (i = 0; i < 16; i++) {
        //查询每个char的8个位，如果不为0,就
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0) { p[0] = c; }
        if ((d & 0x40) != 0) { p[1] = c; }
        if ((d & 0x20) != 0) { p[2] = c; }
        if ((d & 0x10) != 0) { p[3] = c; }
        if ((d & 0x08) != 0) { p[4] = c; }
        if ((d & 0x04) != 0) { p[5] = c; }
        if ((d & 0x02) != 0) { p[6] = c; }
        if ((d & 0x01) != 0) { p[7] = c; }
    }
    return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：在指定位置显示一个字符串
//参数：VRAM初址，窗口宽，待显示的位置，颜色，待显示文字串的初址（这里采用16个char表示一个字符）
//附加：这里采用ASCII,字符串以0x00结尾，要调用putfont8()函数，这里可以和printf()函数连用将数据格式化为字符串类型
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
    extern char hankaku[4096];
    for (; *s != 0x00; s++) {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
    return;
}

//-----------------------------------------------------------------------------------
//显示鼠标指针
//-----------------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////
//功能：准备鼠标指针(16*16)
//参数：存放鼠标颜色信息的字符指针，窗口背景颜色
//附件：先定义一个cursor鼠标效果数组，然后转换为鼠标颜色数组（即，每一点应该涂什么颜色）
void init_mouse_cursor8(char *mouse, char bc)
{
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
    };//16*16的字节的内存
    int x, y;

    for (y = 0; y < 16; y++) {//根据上面的鼠标数组建立一个绘制方案保存在mouse里
        for (x = 0; x < 16; x++) {
            if (cursor[y][x] == '*') {//边缘涂黑色
                mouse[y * 16 + x] = COL8_000000;
            }
            if (cursor[y][x] == 'O') {//内部涂白色
                mouse[y * 16 + x] = COL8_FFFFFF;
            }
            if (cursor[y][x] == '.') {//外部图背景色
                mouse[y * 16 + x] = bc;
            }
        }
    }
    return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：绘制鼠标
//参数：VRAM初址，x轴像素，鼠标指针大小，位置，鼠标绘制方案，bxsize和pxsize大体相同
//附件：鼠标绘制方案即初始化函数得到的mouse，真正用的时候，先初始化，然后就可以用这个函数绘制鼠标啦
void putblock8_8(char *vram, int vxsize, int pxsize,
    int pysize, int px0, int py0, char *buf, int bxsize)
{
    int x, y;
    for (y = 0; y < pysize; y++) {
        for (x = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
    return;
}
