/* In this file, not only have the defination of the function, but also 
hava the description of where the function is.*/

/* asmhead.nas */
struct BOOTINFO { /* 0x0ff0-0x0fff */
    char cyls; /* what's the end of the start zone read the data */
    char leds; /* when boot,the LED's state of the keyboard */
    char vmode; /* GPU mode:how many bits of color */
    char reserve;
    short scrnx, scrny; /* resolution */
    char *vram;
};
#define ADR_BOOTINFO    0x00000ff0
#define ADR_DISKIMG			0x00100000

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);/////////
void store_cr0(int cr0);/////////
void load_tr(int tr);
void asm_inthandler20(void);//定时器中断
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);/////////
void farjmp(int eip,int cs);//任务切换
//void taskswitch3(void);//任务切换
//void taskswitch4(void);//任务切换
void farcall(int eip, int cs);//far 调用
//void asm_cons_putchar(void);//显示一个字符
void asm_hrb_api(void);//调用API

/* fifo.c */
struct FIFO32 {//FIFO缓冲区数据结构
    int *buf;//缓冲区
    int p, q, size, free, flags;//下一个数据的写入地址，下一个数据的读出地址，缓冲区的大小，free是缓冲区没有数据的字节数，flag是是否溢出
		struct TASK *task;//当FIFO中写数据的时候将任务唤醒，用于记录要唤醒任务的信息
};

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);//缓冲区结构体指针，大小，缓冲区开始位置，有数据写入的时候要唤醒任务的任务
int fifo32_put(struct FIFO32 *fifo, int data);//往缓冲区内插入一个数据，当有任务处于休眠的时候要唤醒S
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize);
#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

/* dsctbl.c about GDT IDT that's Global Descriptor Table and Interrupt Descriptor Table*/
struct SEGMENT_DESCRIPTOR {//8 bytes segment infomation,total 8192 parts [here similar to the method of the setting palette] 
    short limit_low, base_low;//address of segment base[high:mid:low=1:1:2]=4 bytes =32 bits
    char base_mid, access_right;//segment limit[high:low=1:2],only 20 bits = high byte's low 4 bits + low 2bytes' 16bits
    char limit_high, base_high;//segment property access[limit_high:right=1:1],only 12 bits = limit_high's high 4 bits + access_right's 8 bits
};
//PS 1):segment limit equals the number of GDT's effective bytes -1 
//PS 2):the segment limit just only has 20 bits, which can represent 1MB, and the segment property has 1 bit flag, 
//if this flag =1,and the limit's unit uses page to replace byte(here 1 page = 4kb) 
//PS 3):the segment property has 16 bits liking that:xxxx0000 xxxxxxxx 
//the high 4 bits are extended access
//the low 8 bits are:
//    0x00:unused description table
//    0x92:system exclusive,readable and writable,non-executable
//  0x9a:system exclusive,readable and non-writable,executable
//  0xf2:application useing,readable and writable,non-executable
//  0xfa:application useing,readable and non-writable,executable

struct GATE_DESCRIPTOR {//
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */
void init_pic(void);
void inthandler27(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo,int data0);//初始化键盘控制电路
extern struct FIFO32 *keyfifo;
extern int keydata0;
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064

/* mouse.c */
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo,int data0,struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);
extern struct FIFO32 *mousefifo;
extern int mousedata0;

/* memory.c */
#define MEMMAN_FREES		4090	/* 大约是32KB */
#define MEMMAN_ADDR			0x003c0000
struct FREEINFO {	/* 可用信息 */
	unsigned int addr, size;
};
struct MEMMAN {		/* 内存管理 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS		256
struct SHEET {//图层结构体
	unsigned char *buf;//所描绘内容的地址
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;//图层大小，图层坐标，透明色色号，土层高度，存放有关图层的设定信息
	struct SHTCTL *ctl;
};
struct SHTCTL {//图层管理结构体
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};//top存放最上面图层的高度，sheet0图层顺序混乱，要按照升序排列，然后将地址写入sheets中，方便使用
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/* timer.c */
#define MAX_TIMER     500			//最多500个定时器
struct TIMER{
	struct TIMER *next;//用来指下一个即将超时的定时器地址
	unsigned int flags;//flags记录各个寄存器状态
	unsigned int timeout;//用来记录离超时还有多长时间，一旦这个剩余时间为0，程序就往FIFO缓冲区里发送数据，定时器就是用这种方法通知HariMain时间到了
	struct FIFO32 *fifo;//消息队列
	int data;//该定时器标志，用来向消息队列写的标志信息
};
struct TIMERCTL {
	unsigned int count, next;//next是下一个设定时间点，count是累加时间轴
	struct TIMER *t0;//记录按照某种顺序存好的定时器地址，头指针
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);//定时器初始化100hz
struct TIMER *timer_alloc(void);//分配定时器，遍历所有找到第一个没有使用的分配
void timer_free(struct TIMER *timer);//释放定时器
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);//初始化定时器，fifo和标志符data
void timer_settime(struct TIMER *timer, unsigned int timeout);//定时器设置，设定剩余时间
void inthandler20(int *esp);//定时器中断函数

/* mtask.c 任务切换相关*/
#define MAX_TASKS					1000   /* 最大任务数量 */
#define TASK_GDT0					3			 /* 定义从GDT的几号开始分配给TSS */
#define MAX_TASKS_LV	100        //每个level最多100个任务
#define MAX_TASKLEVELS	10			 //最多10个level

struct TSS32 {//task status segment 任务状态段
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;//保存的不是寄存器的数据，而是与任务设置相关的信息，在执行任务切换的时候这些成员不会被写入（backlink除外，某些情况下会被写入）
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;//32位寄存器
	int es, cs, ss, ds, fs, gs;//16位寄存器
	int ldtr, iomap;//有关任务设置部分
};
struct TASK {
	int sel, flags; /* sel用来存放GDT的编号 */
	int level, priority;
	struct FIFO32 fifo;//把FIFO绑定到任务里
	struct TSS32 tss;
};
struct TASKLEVEL {
	int running; /* 正在运行的任务量数 */
	int now; /* 这个变量用来记录当前正在运行的任务是哪一个 */
	struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL {
	int now_lv; /* 正在运行的level */
	char lv_change; /* 在下次任务切换时是否需要改变LEVEL */
	struct TASKLEVEL level[MAX_TASKLEVELS];//最多10个level
	struct TASK tasks0[MAX_TASKS];
};
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);//初始化任务控制
struct TASK *task_alloc(void);//分配一个任务
void task_run(struct TASK *task, int level, int priority);//设置一个任务的LEVEL和优先级
void task_switch(void);//如果LEVEL改变了要改变当前的LEVEL然后从当前的LEVEL中找到当前任务进行切换
void task_sleep(struct TASK *task);//删除任务的时候要考虑LEVEL的变化~

/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);//ACT为0时窗口标题栏是灰色，是1时是蓝色
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);//字符串显示
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);//文本输入框
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);//画标题，供下面的函数引用

/* console.c */
struct CONSOLE {
	struct SHEET *sht;//console窗口图层
	int cur_x, cur_y, cur_c;//当前位置和颜色
};
void console_task(struct SHEET *sheet, unsigned int memtotal);//console任务
void cons_putchar(struct CONSOLE *cons, int chr, char move);//显示一个字符
void cons_newline(struct CONSOLE *cons);//换行与滚动
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);//运行命令行，调用下面的命令行函数
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline);
//void cmd_hlt(struct CONSOLE *cons, int *fat);
int  cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);//只要输入文件名就能执行该文件，应用程序
void hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);//API处理程序


/* file.c */
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};//修改MakeFile中的语句，将haribote.sys、ipl10.nas、make.bat这3个文件制成操作系统镜像
//这样磁盘映像中0x002600字节以后的部分8(文件名)+3(文件格式)+1(文件属性)+10(留待)+2(时间)+2(日期)+2(簇号)+4(大小)字节
void file_readfat(int *fat, unsigned char *img);//将磁盘映像中的FAT解压缩，fat存放磁盘位置
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);//将文件的内容读入内存，这样文件的内容已经排列为正确的顺序
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);//查找文件

