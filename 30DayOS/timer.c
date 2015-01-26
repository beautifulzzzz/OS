/* PIT 定时器 */

#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

struct TIMERCTL timerctl;
//struct TIMERCTL timerctl;//计数器结构体实例化
#define TIMER_FLAGS_ALLOC				1		//已配置状态
#define TIMER_FLAGS_USING				2		//定时器运行中

/////////////////////////////////////////////////////////////////////////////////////
//功能：定时器初始化，要3次OUT指令,(0x34->0x34)(中断周期低8位->0x40)(高8位->0x40)
//参数：
//附加：设置结果为主频/设置数，这里中断周期设置为0x2e9c,大约为100hz,具体搜：IRQ0中断周期变更PIT
void init_pit(void)
{
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count=0;//初始化计数为0
	for(i=0;i<MAX_TIMER;i++){//初始化所有定时器未使用
		timerctl.timers0[i].flags=0;//未使用
	}
	t=timer_alloc();//取得一个
	t->timeout=0xffffffff;
	t->flags=TIMER_FLAGS_USING;
	t->next=0;//末尾
	timerctl.t0=t;//现在就一个
	timerctl.next=0xffffffff;//下一个计时器为哨兵，所以下一个为无穷大
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：分配定时器
//参数：
struct TIMER *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {//从开始开始找没有使用的定时器，找到后设置为分配状态，反回
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	return 0; /* 没有找到 */
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：释放定时器，直接把标志位设为0即可
//参数：
void timer_free(struct TIMER *timer)
{
	timer->flags = 0; /* 未使用 */
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：初始化定时器，赋值fifo,设置标志位
//参数：
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：设置timer
//参数：输入定时时间
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e;
	struct TIMER *t,*s;
	timer->timeout = timeout + timerctl.count;//当前时间+定时器定时时间
	timer->flags = TIMER_FLAGS_USING;//设置成正在使用
	e = io_load_eflags();//保存寄存器，关中断
	io_cli();

	t=timerctl.t0;
	if (timer->timeout <= t->timeout) {
		/* 插入最前面的情况 */
		timerctl.t0 = timer;
		timer->next = t; /* 下面是设定t */
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	/* 搜寻插入位置 */
	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			/* 插入s和t之间 */
			s->next = timer; /* s下一个是timer */
			timer->next = t; /* timer下一个是t */
			io_store_eflags(e);
			return;
		}
	}
	return;
}

/////////////////////////////////////////////////////////////////////////////////////
//功能：定时器中断处理程序，和键盘鼠标中断类似
//参数：
void inthandler20(int *esp)
{
	char ts=0;//标记mt_timer任务切换计时器是否超时，如果直接在else语句直接调用任务切换会出现错误，因为这个中断处理还没完成
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);	/* 把IRQ-00信号接受完了的信息通知给PIC */
	timerctl.count++;
	if (timerctl.next > timerctl.count) {//如果下一个还没计数完毕就直接返回
		return;
	}
	timer = timerctl.t0; //把最前面的地址赋址给timer
	for (;;) {
		if (timer->timeout > timerctl.count) {
			break;
		}//从前往后遍历，一旦发现有计时未完成的计时器就跳出循环
		/*除了上面的情况，都是定时已达的定时器*/
		timer->flags = TIMER_FLAGS_ALLOC;
		if(timer != task_timer){
			fifo32_put(timer->fifo, timer->data);
		}else{
			ts=1;/* mt_timer超时 */
		}
		timer = timer->next;//下一个定时器的地址赋址给timer
	}
	timerctl.t0 = timer;//新移位
	timerctl.next = timer->timeout;//timectl.next设定
	if(ts!=0){
		task_switch();
	}
	return;
}

