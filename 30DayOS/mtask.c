/* 任务管理相关程序 */

#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

/////////////////////////////////////////////////////////////////////////////////////
//功能：用来返回现在活动中的struct TASK的内存地址
//参数：
struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：向struct TASKLEVEL中添加一个任务
//参数：
void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];//获取task的level
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2; /* 活动中 */
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：从struct TASKLEVEL中删除一个任务，就是线性数组中删除一个数据类似的数据结构
//参数：
void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];//获取task的level

	/* 寻找task所在的位置 */
	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--; /* 需要移动成员，所以要做相应的处理 */
	}
	if (tl->now >= tl->running) {//超出最大，回到最小
		tl->now = 0;
	}
	task->flags = 1; /* 休眠中 */

	/* 移动 */
	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：用来在任务时决定接下来切换到哪个LEVEL
//参数：
void task_switchsub(void)
{
	int i;
	for (i = 0; i < MAX_TASKLEVELS; i++) {//寻找最上层的level
		if (taskctl->level[i].running > 0) {//从上层遍历，如果这一层当前运行的任务存在就返回这个level
			break; 
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：闲置，不工作，只执行HLT
//参数：
void task_idle(void)
{
	for (;;) {
		io_hlt();
	}
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：初始化任务控制
//参数：
//返回：返回一个内存地址，意思是现在正在运行这个程序，已经变成一个任务
struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));//TASKCTL是个很大的结构体，所以要申请一个内存空间
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);//定义在gdt的号，段长限制为103字节
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {//把每一层的level全都设置为0
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}
	
	task = task_alloc();
	task->flags = 2; /* 活动中标志 */
	task->priority = 2; //任务优先级//0.02s定时器
	task->level = 0;	/* 第0层 */
	task_add(task);		//加入level中
	task_switchsub();	/* 用来任务切换时决定接下来切换到哪个LEVEL */
	load_tr(task->sel);
	//向TR寄存器写入这个值，因为刚才把当前运行任务的GDT定义为3号，TR寄存器是让CPU记住当前正在运行哪一个任务
	//每当进行任务切换时，TR寄存器的值也会自动变换，task register
	//每次给TR赋值的时候，必须把GDT的编号乘以8
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);
	
	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 1);//将idle放在最低优先级，当没有其他任务时，就处于这个任务，休眠
	
	return task;//这样开始的时候level0里只有一个任务
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：任务分配[遍历所有的任务，发现任务处于空闲状态的进行初始化]
//参数：
struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; /* 正在使用标志 */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0; /* 这里先设置为0 */
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;//先这样设置
			task->tss.iomap = 0x40000000;
			return task;
		}
	}
	return 0; /* 全部都正在使用 */
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：在没有改变之前，task_run中下一个要切换的任务是固定不变的，不过现在就不同了，
//如果本次task_run启动一个比当前活动中的任务LEVEL更高的任务，那么下次任务切换时，就得无条件的切换到更高优先级的LEVEL中
//此外，如果当前任务中的LEVEL被下调，那么就得把其他LEVEL的有先任务放在前面
//综上所述：我们需要再下次切换时先检查LEVEL,因此将lv_change置为1
void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; /* 不改变level */
	}
	if (priority > 0) {
		task->priority = priority;
	}

	if (task->flags == 2 && task->level != level) { /* 改变活动中的LEVEL */
		task_remove(task); /* 这里执行之后flag的值会变为1，于是下面的语句也会被执行 */
	}
	if (task->flags != 2) {
		/* 从休眠状态唤醒 */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* 下次切换任务时检查LEVEL */
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：running为1的时候不用进行任务切换，函数直接结束，当running大于2的时候，先把now加1
//再把now代表的任务切换成当前的任务，最后再将末尾的任务移到开头
//参数：
void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];//当前任务的LEVEL
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) {
		tl->now = 0;
	}
	if (taskctl->lv_change != 0) {//LEVEL之间变换了
		task_switchsub();//用来在任务时决定接下来切换到哪个LEVEL,直接从头开始找，找到第一个LEVEL中有任务的返回
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：任务休眠，从任务数组中删除该任务，如果处于正在运行的任务，就让其休眠
//参数：
void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		/* 如果处于活动状态 */
		now_task = task_now();
		task_remove(task); /* 执行此语句的话，flags将被置为1 remove其实就是从数组中删除一个*/
		if (task == now_task) {
			/* 如果让自己休眠需要进行任务切换 */
			task_switchsub();
			now_task = task_now(); /* 在设定后获取当前任务的值 */
			farjmp(0, now_task->sel);
		}
	}
	return;
}
