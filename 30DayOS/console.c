
#include "bootpack.h"
#include <string.h>
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////////////
//功能：consloe窗口任务
//参数：
void console_task(struct SHEET *sheet, unsigned int memtotal)
{
	struct TIMER *timer;
	struct TASK *task = task_now();//需要自己休眠，就要获取自己的任务地址
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;//内存
	int i, fifobuf[128], *fat = (int *) memman_alloc_4k(memman, 4 * 2880);//先分配一个内存用来存解压后的FAT
	struct CONSOLE cons;//consloe类
	char cmdline[30];//命令行
	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	
	fifo32_init(&task->fifo, 128, fifobuf, task);
	timer = timer_alloc();//分配时钟
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));//读取第一个FAT解压为fat中，第一个位于0x000200~0x0013ff
	
	cons_putchar(&cons, '>', 1);//显示提示符
	
	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {//没有数据就休眠
			task_sleep(task);
			io_sti();
		} else {//有数据就处理
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1) {//光标定时器 
				if (i != 0) {
					timer_init(timer, &task->fifo, 0); 
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(timer, &task->fifo, 1); 
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(timer, 50);
			}
			if(i==2){//光标ON
				cons.cur_c = COL8_FFFFFF;
			}
			if(i==3){//光标OFF
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				cons.cur_c = -1;
			}
			if(256 <= i && i <= 511) { /* 键盘数据，通过任务A */
				if (i == 8 + 256) {
					if (cons.cur_x > 16) {//退格键
						/* 用空白擦除光标后将光标前移一位 */
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if(i == 10+256){//回车键
					//用空格将光标擦除
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);	/* 运行命令行 */
					/* 显示提示符 */
					cons_putchar(&cons, '>', 1);
				}else{
					/* 一般字符 */
					if (cons.cur_x < 240) {
						/* 显示一个字符后将光标后移一位 */
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			//重新显示光标
			if (cons.cur_c >= 0) {
				boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
			}
			sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：在console里输入字符
//参数：move为0时光标不后移
void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	//制表符是用来对其字符的显示位置，遇到制表符就显示4个空格是不正确的，而是在当前位置和下一个制表符之间填充空格~
	if (s[0] == 0x09) {	/* 制表符 */
		for (;;) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {//超出就换行
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {//减8就是减去边界，&0x1f是求其%32的余数，每隔制表符为4字符
				break;
			}
		}
	} else if (s[0] == 0x0a) {	/* 换行 */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	/* 回车 */
		/* 暂时不做任何事 */
	} else {	/* 一般字符 */
		putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		if (move != 0) {
			/* move为0时光标不后移 */
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
		}
	}
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：换行与滚动，独立出来~
//参数：
void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	//换行与滚动屏幕
	if (cons->cur_y < 28 + 112) {
		cons->cur_y += 16; /* 换行 */
	} else {
		/* 滚动 */
		for (y = 28; y < 28 + 112; y++) {//向上移动一行
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
			}
		}
		for (y = 28 + 112; y < 28 + 128; y++) {//将最后一行覆盖（这里的行是6个）
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	}
	cons->cur_x = 8;
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：运行窗口命令
//参数：
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal)
{
	if (strcmp(cmdline, "mem") == 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "cls") == 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "dir") == 0) {
		cmd_dir(cons);
	} else if (strncmp(cmdline, "type ", 5) == 0) {
		cmd_type(cons, fat, cmdline);
	} else if (strcmp(cmdline, "hlt") == 0) {
		cmd_hlt(cons, fat);
	} else if (cmdline[0] != 0) {
		/* 不是命令也不是空行 */
		putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "Bad command.", 12);
		cons_newline(cons);
		cons_newline(cons);
	}
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：mem命令,获取系统内存信息并显示
//参数：
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[30];
	sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
	putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
	cons_newline(cons);
	sprintf(s, "free %dKB", memman_total(memman) / 1024);
	putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
	cons_newline(cons);
	cons_newline(cons);
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：cls命令,清屏
//参数：
void cmd_cls(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28;
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：dir命令,查看文件
//参数：
void cmd_dir(struct CONSOLE *cons)
{
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {//没有任何文件信息
			break;
		}
		if (finfo[i].name[0] != 0xe5) {//表明该文件已经被删除0xe5
			if ((finfo[i].type & 0x18) == 0) {//一般文件不是0x20就是0x00
				sprintf(s, "filename.ext   %7d", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
				cons_newline(cons);
			}
		}
	}
	cons_newline(cons);
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：type命令,查看文件内容
//参数：
void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search(cmdline + 5, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	char *p;
	int i;
	if (finfo != 0) {
		/* 找到文件的情况 */
		p = (char *) memman_alloc_4k(memman, finfo->size);//分配一个和文件大小一样的内存
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));//将文件的内容读入内存，这样文件的内容已经排列为正确的顺序
		for (i = 0; i < finfo->size; i++) {//输出内容，掉用cons_putchar函数
			cons_putchar(cons, p[i], 1);
		}
		memman_free_4k(memman, (int) p, finfo->size);//释放内存
	} else {
		/* 没有找到文件的情况 */
		putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
		cons_newline(cons);
	}
	cons_newline(cons);//新行
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：hlt命令,运行hlt程序
//参数：
void cmd_hlt(struct CONSOLE *cons, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search("HLT.HRB", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	char *p;
	if (finfo != 0) {
		/* 有文件 */
		p = (char *) memman_alloc_4k(memman, finfo->size);//申请一个内存地址大小一样的p
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));//读取FAT到p
		set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER);//将hlt.hrb读入内存后将其注册为GDT1003号，因为1~2号右dsctbl.c用，而3~1002号由mtask.c
		farjmp(0, 1003 * 8);//然后调用跳转运行
		memman_free_4k(memman, (int) p, finfo->size);//释放内存
	} else {
		/* 无文件 */
		putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
		cons_newline(cons);
	}
	cons_newline(cons);
	return;
}
