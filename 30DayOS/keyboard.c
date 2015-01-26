/* 键盘处理函数文件 */

#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

/////////////////////////////////////////////////////////////////////////////////////
//功能：PS/2的键盘中断，一会在汇编中调用这个函数实现真正的中断：_asm_inthandler21 
void inthandler21(int *esp)
{
	int data;
	io_out8(PIC0_OCW2, 0x61);	/* IRQ-01庴晅姰椆傪PIC偵捠抦 */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(keyfifo, keydata0+data);
	return;
}

#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
/////////////////////////////////////////////////////////////////////////////////////
//功能：等待键盘控制电路准备完毕
//参数：无
//附加：如果键盘控制电路可以接受CPU指令，CPU从设备号码0x0064处所读取的数据的倒数第二位应该是0，否则就一种循环等待
void wait_KBC_sendready(void)
{
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo,int data0)//初始化键盘控制电路
{
	/*将FIFO缓冲区里的信息保存到全局变量里*/
	keyfifo=fifo;
	keydata0=data0;
	//键盘控制器初始化
  wait_KBC_sendready();//让键盘控制电路做好准备，等待控制指令的到来
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);//键盘模式设置指令
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, KBC_MODE);//鼠标模式设置指令
  return;
}
