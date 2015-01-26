/* bootpack */

#include "bootpack.h"



#define KEYCMD_LED		0xed

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct FIFO32 fifo, keycmd;
	//定时器FIFO，keycmd不是用来接收中断请求的，而是管理由任务A向键盘控制器发送数据的顺序的。
	//如果有数据要发送键盘控制器，首先会在这个keycmd中累积起来
	char s[40];
	int fifobuf[128],keycmd_buf[32];
	int mx, my, i ,cursor_x, cursor_c;//cursor_x是记录光标位置的变量，cursor_c表示光标现在的颜色
	unsigned int memtotal;//总内存大小
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	static char keytable0[0x80] = {//键盘解码
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_to = 0, key_shift = 0, key_leds=(binfo->leds>>4)&7, keycmd_wait=-1;
	//用来记录键盘key应该发送到to哪里，为0则发送到任务A，为1则发送到任务B
	//左shitf按下置1，右按下置2，都按置3，不按置0，为0时用keytable0[]解码
	//Caps Lock的状态保存在binfo的led中，第4位srollLock状态，第5位NumLock,第6位CapsLock
	//大小写切换有如下规律：
	//CapLock[ON] && Shitf[ON]小写输入  其它自己推~
	//keycmd_wait用来表示向键盘控制器发送数据的状态，-1表示键盘控制器处于通常状态；
	//当不为-1时,表示键盘控制器正在等待发送的数据，这时要发送的数据被保存在keycmd_wait中
	
	struct SHTCTL *shtctl;//图层管理
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;//4个图层
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	struct TASK *task_a , *task_cons;//建立任务
	struct TIMER *timer;//一个定时器

	
	init_gdtidt();//在dsctbl.c中，负责分区和中断分区初始化[包括键盘和鼠标中断设定]
	init_pic();//在int.c中，负责中断初始化（硬件）
	io_sti();//在naskfunc.nas中，仅仅执行STI指令，是CLI的逆指令，前者是开中断，后者是禁止中断
	/*
	同一占用一个fifo,这里：
	0~1   			光标闪烁用定时器
	3     			3秒定时器
	10    			10秒定时器
	256~511			键盘输入（从键盘控制器读入的值再加上256)
	512~767			鼠标输入（从键盘控制器读入的值再加上512)
	*/
	fifo32_init(&fifo, 128, fifobuf,0);//初始化fifo，先让最后一个task参数为0，我们现在还没有初始化完成a任务
	init_pit();//负责计时器初始化100hz
	init_keyboard(&fifo, 256);//初始化键盘控制电路//在fifo.c中，负责缓冲区初始化（缓冲区结构体，大小，缓冲区首址）
	enable_mouse(&fifo, 512, &mdec);//使能鼠标
	/*这里IMR是(interrupt mask register),意思是“中断屏蔽寄存器”，是8位寄存器，分别对应8路IRQ信号，如果一路是1则该路被屏蔽，因为键盘中断是IRQ1,鼠标中断是IRQ12,且PIC分主从2个，从PIC连接主PIC的IRQ2,所以想要有鼠标和键盘中断，要PIC0的IRQ1和IRQ2，和PIC1的IRQ4*/
	io_out8(PIC0_IMR, 0xf8); /* (11111000) *///PIT,PIC1,键盘许可
	io_out8(PIC1_IMR, 0xef); /* (11101111) */
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	
	//memman需要32KB内存
	memtotal = memtest(0x00400000, 0xbfffffff);//计算总量memtatal
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff 将现在不用的字节以0x1000个字节为单位注册到memman里*/
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	init_palette();//调色板
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);//图层初始化函数
	task_a = task_init(memman);//初始化任务a//初始化任务管理器，task_init会返回自己的构造地址，我们将这个地址存入fifo.task
	fifo.task = task_a;//可以自动管理，待唤醒的task（//记录休眠任务名）
	task_run(task_a, 1, 2);//将任务A的LEVEL设置为1，优先级2，B的3个任务LEVEL是2，所以优先级低
	
	/* sht_back */
	sht_back  = sheet_alloc(shtctl);//分配一个背景窗口
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);//为背景窗口和普通小窗口分配缓存空间
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 设定涂层缓冲区的大小和透明色的函数 */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);//初始化屏幕，画矩形，形成最初的窗口界面
	
	/* sht_cons */
	sht_cons = sheet_alloc(shtctl);//分配窗口
	buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);//为窗口分配缓存
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); //设定窗口缓冲区
	make_window8(buf_cons, 256, 165, "console", 0);//就像制作背景和鼠标一样，先准备一张图，然后在图层内描绘一个貌似窗口的图就可以了
	make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);//文本编辑区
	//任务相关
	task_cons = task_alloc();//分配一个任务
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;//分配栈
	//要为任务B专门分配栈，直接用任务A的栈就会乱成一团糟
	//这里任务B的函数式是带参数的，这里采用汇编函数参数传递的思想，传进任务B函数的参数其实就在[ESP+4]
	//这里用申请的内存+64*1024要减8因为*((int *) (task_b_esp + 4)) = (int) sht_back;这句将sht_back写入task_b_esp + 4
	//从这个地址开始向后写4字节的sht_back的值，正好在分配的内存范围
	task_cons->tss.eip = (int) &console_task;//任务入口地址
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	*((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;//参数传递
	*((int *) (task_cons->tss.esp + 8)) = memtotal;//将全部内存大小传过去
	task_run(task_cons, 2, 2); /* level=2, priority=2 */
	
	/* sht_win */
	sht_win   = sheet_alloc(shtctl);//分配一个小窗口
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);//分配窗口的缓存
	sheet_setbuf(sht_win, buf_win, 144, 52, -1); /* 设定涂层缓冲区的大小和透明色的函数 */
	make_window8(buf_win, 144, 52, "task_a", 1);//就像制作背景和鼠标一样，先准备一张图，然后在图层内描绘一个貌似窗口的图就可以了
	make_textbox8(sht_win, 8, 28, 127, 16, COL8_FFFFFF);//窗口上的文件编辑部分[在shtwin中的左上角位置和大小]
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	timer = timer_alloc();//分配一个定时器
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);
	
	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);//分配一个鼠标窗口
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);//准备鼠标指针(16*16),99是窗口背景颜色
	mx = (binfo->scrnx - 16) / 2; /* 计算鼠标初始位置 */
	my = (binfo->scrny - 28 - 16) / 2;
	
	//移动窗口初始位置以及设置层叠顺序
	sheet_slide(sht_back, 0, 0);//上下左右移动窗口,即移动窗口至0,0
	sheet_slide(sht_cons, 122,  140);
	sheet_slide(sht_win,  180,  210);//移动任务a窗口
	sheet_slide(sht_mouse, mx, my);//移动鼠标窗口
	sheet_updown(sht_back,     0);//设置窗口对的高度，背景在最下面
	sheet_updown(sht_cons,     1);
	sheet_updown(sht_win,      4);
	sheet_updown(sht_mouse,    5);
	
//	sprintf(s, "(%3d, %3d)", mx, my);//显示鼠标位置
//  putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
//	sprintf(s, "memory %dMB   free : %dKB",
//			memtotal / (1024 * 1024), memman_total(memman) / 1024);
//	putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);
	
	//为了避免和键盘当前状态冲突，在一开始先进行设置
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	
	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/* 如果尚存在向键盘控制器发送的数据，则发送它 */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			task_sleep(task_a);//应该在中断屏蔽的时候进入休眠状态
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (256 <= i && i <= 511) {//键盘数据
				//sprintf(s, "%02X", i - 256);//减去256
				//putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);//清，显，刷
				
				if (i < 0x54 + 256) {//将按键编码转换为字符编码
					if(key_shift==0){//如果shift没按下就用keytable0解码
						s[0]=keytable0[i-256];
					}else{
						s[0]=keytable1[i-256];
					}
				}else{
					s[0]=0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') {	/* 当输入字符为英文字母时，大小写切换 */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	/* 将大写字母转换为小写字母 */
					}
				}				
				if(s[0]!=0){//一般字符，通过上面s[0]的设置
					if(key_to==0){//发送数据给窗口A
						if (cursor_x < 128) {//
						/* 显示一次就前移一次光标 */
							s[1] = 0;
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;//记录光标位置
						}
					}else{//发送给console
							fifo32_put(&task_cons->fifo, s[0] + 256);
					}
				}
				if (i == 256 + 0x0e) { /* 退格键 */
					/* 用空格键把光标消去后，后移1次光标 */
					if(key_to==0){//发送给窗口A
						if(cursor_x > 8){
							putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					}else{//发送给console
						fifo32_put(&task_cons->fifo, 8 + 256);
					}
				}
				if(i == 256 + 0x0f){//Tab键，负责切换窗口，这里只是标题颜色做了改变
					if (key_to == 0) {
						key_to = 1;
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  0);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
						cursor_c = -1; /* 不显示光标 */
						boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
						fifo32_put(&task_cons->fifo,2);//命令行窗口光标ON[从任务A向console传命令]
					} else {
						key_to = 0;
						make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  1);
						make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
						cursor_c = COL8_000000; /* 显示光标 */
						fifo32_put(&task_cons->fifo,3);//命令行窗口光标OFF
					}
					sheet_refresh(sht_win,  0, 0, sht_win->bxsize,  21);
					sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
				}
				if (i == 256 + 0x2a) {	/* 左shift ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36) {	/* 右shift ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) {	/* 左shift OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) {	/* 右shift OFF */
					key_shift &= ~2;
				}
				//当接收到的是各种锁定键盘的命令时，将控制消息写进keycmdFIFO中，用来控制键盘上面的锁定LED
				if (i == 256 + 0x3a) {	/* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) {	/* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) {	/* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0xfa) {	/* 键盘成功接收到数据 */
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) {	/* 键盘没有成功接收到数据 */
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				if (i == 256 + 0x1c) {  /* 回车键 */
					if (key_to != 0){//发送命令至窗口
						fifo32_put(&task_cons->fifo,10+256);
					}					
				}
				/* 重新显示光标 */
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				}
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}else if (512 <= i && i <= 767) {//鼠标数据
				//已经收集了3字节的数据，所以显示出来
				if (mouse_decode(&mdec, i - 512) != 0) {
//					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
//					if ((mdec.btn & 0x01) != 0) {
//						s[1] = 'L';
//					}
//					if ((mdec.btn & 0x02) != 0) {
//						s[3] = 'R';
//					}
//					if ((mdec.btn & 0x04) != 0) {
//						s[2] = 'C';
//					}
//					putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);//清，显，刷
					/* 移动鼠标 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
//					sprintf(s, "(%3d, %3d)", mx, my);
//					putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);//清，显，刷
					sheet_slide(sht_mouse, mx, my);
					//移动窗口计算
					if((mdec.btn & 0x01)!=0){
						sheet_slide(sht_win,mx-80,my-80);
					}//按下左键移动sht_win
				}
			}else if (i<=1) { //光标用定时器
				if (i != 0) {
					timer_init(timer, &fifo, 0); 
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				} else {
					timer_init(timer, &fifo, 1); 
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				}
				timer_settime(timer, 50);
				if (cursor_c >= 0) {
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
	}
}
