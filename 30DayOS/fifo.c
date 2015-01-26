/* FIFO用来存放鼠标和键盘信息的链表
其实就是用数组实现的循环链表，这里原著处理的不好，
建议用数据结构里的知识来处理 */

#include "bootpack.h"

#define FLAGS_OVERRUN        0x0001

/////////////////////////////////////////////////////////////////////////////////////
//功能：缓冲区初始化
//参数：缓冲区结构体指针，大小，缓冲区开始位置，有数据写入的时候要唤醒任务的任务
//附加：如果不想使用任务自动唤醒功能可以task=0
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size; 
    fifo->flags = 0;
    fifo->p = 0; //写入位置
    fifo->q = 0; //读取位置
    fifo->task=task;//有数据写入时需要唤醒任务
    return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：往缓冲区内插入一个数据，当有任务处于休眠的时候要唤醒
//参数：
int fifo32_put(struct FIFO32 *fifo, int data)
{
    if (fifo->free == 0) {//溢出
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size) {//当插入位置到达最后时再返回第一个位置
        fifo->p = 0;
    }
    fifo->free--;
    if(fifo->task!=0){//如果设置了有唤醒任务就唤醒
    	if(fifo->task->flags!=2){//如果处于休眠状态
    		task_run(fifo->task, -1, 0); //将任务唤醒
    	}
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：从缓冲区取出一个数据
//参数：
int fifo32_get(struct FIFO32 *fifo)
{
    int data;
    if (fifo->free == fifo->size) {//没有数据
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {//如果取出数据的位置到达最后了就从第一个位置开始
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：获取缓冲区状态，数据量
//参数：
int fifo32_status(struct FIFO32 *fifo)
{
    return fifo->size - fifo->free;
}
