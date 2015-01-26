/* 鼠标相关函数 */

#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

/////////////////////////////////////////////////////////////////////////////////////
//功能：PS/2的鼠标中断，一会在汇编中调用这个函数实现真正的中断：_asm_inthandler2c
void inthandler2c(int *esp)
{
	unsigned char data;
	io_out8(PIC1_OCW2, 0x64);	 /* IRQ-12受付完了をPIC1に通知 */
	io_out8(PIC0_OCW2, 0x62);	 /* IRQ-02受付完了をPIC0に通知 */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, data+mousedata0);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
/////////////////////////////////////////////////////////////////////////////////////
//功能：使能鼠标
//参数：
//附加：这个函数和init_keyboard十分相似，不同的在于输入的数据不同
//如果向键盘控制电路发送指令0xd4,下一数据就会自动发送给鼠标，利用这一特性来发送激活鼠标的指令
//其中答复消息为0xfa
void enable_mouse(struct FIFO32 *fifo,int data0,struct MOUSE_DEC *mdec)
{
	//将FIFO缓冲区信息保存到全局变量
	mousefifo=fifo;
	mousedata0=data0;
	//鼠标有效
  wait_KBC_sendready();//让键盘控制电路做好准备，等待控制指令的到来
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
  //顺利的话，ACK(0xfa)会被发送
  mdec->phase = 0;//等待鼠标0xfa阶段
  return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：鼠标信息解码程序，把信息保存在鼠标结构体中
//参数：鼠标结构体，从缓冲区读取的数据
//附加：首先把最初读到的0xfa舍掉，之后从鼠标那里送过来的数据都应该是3个字节一组的，所以每当数据累计3字节时候就作相应处理[这里返回1]
//最后将鼠标的点击信息放在btn中，将鼠标的移动信息保存在x,y中
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		 /*等待鼠标的0xfa状态*/
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/*等待鼠标的第一字节*/
		if ((dat & 0xc8) == 0x08) {
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if (mdec->phase == 2) {
		/*等待鼠标的第二字节*/
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		/*等待鼠标的第三字节，并作处理*/
		mdec->buf[2] = dat;
		mdec->phase = 1;//注意这里是恢复到1不是0，见附加说明，刚开始舍去0xfa以后每次3字节
    /*下面是根据buf的3个数据进行计算鼠标信息
    这里btn是buf[0]的低3位，最低位表示鼠标左击，中间表示鼠标中间被击，第3位表示鼠标右击
    鼠标的x,和y坐标基本和buf[1],buf[2]相等，但是要根据buf[0]的前半部分作相应的变化：要么第8位及8位以后的全部设成1，或全部保留为0
    */
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y; /* 因为鼠标和屏幕的方向恰好相反，所以这里取反*/
		return 1;
	}
	return -1; 
}
