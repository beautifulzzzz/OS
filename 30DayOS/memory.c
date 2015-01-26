/* 内存相关
策略是先写入再读出看看是否一样，来确定是否正常，但是486以上的有高速缓存，所以要关闭高速缓存才行
 */

#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/////////////////////////////////////////////////////////////////////////////////////
//参数：开始位置和结束位置
//附加：（1）最初对EFLAGS处理，是检查CPU是486还是386.如果是486以上，EFLAGS寄存器的第18位应该是AC标志位
//如果不是386就没有这一位，第18位一直为0，这里我们有意识的把1写到这一位，然后再读EFLAGS的值，继而
//检查AC标志位是否为1.最后将AC标志位重置为0。（2）为了禁止缓存，需要对CR0寄存器的某一位置0，这里用的
//是会编写的load_cr0和store_cr0
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386偐丄486埲崀側偺偐偺妋擣 */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386偱偼AC=1偵偟偰傕帺摦偱0偵栠偭偰偟傑偆 */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* 僉儍僢僔儏嬛巭 */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* 僉儍僢僔儏嫋壜 */
		store_cr0(cr0);
	}

	return i;
}
/*
/////////////////////////////////////////////////////////////////////////////////////
//功能：检查区域内的内存是否有效,因为c编译器会清除掉一些东西，所以就只能用汇编写，见naskfunc.nas
//参数：起始位置
//附加：写入取反看看修改没，再取反看看修改没，如果两次都修改就认为是能够使用的内存
unsigned int memtest_sub(unsigned int start,unsigned int end)
{
	unsigned int i,*p,old,pat0=0xaa55aa55,pat1=0x55aa55aa;
	for(i=start;i<=end;i+=4){
		p=(unsigned int *)i;
		old=*p;
		*p=pat0;
		*p^=0xffffffff;
		if(*p!=pat1){
		not_memory:
			*p=old;
			break;			
		}
		*p^=0xffffffff;
		if(*p!=pat0){
			goto not_memory;
		}
		*p=old;
	}
	return i;
}
*/


/////////////////////////////////////////////////////////////////////////////////////
//功能：初始化内存管理
//参数：
void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* 可用信息数目 */
	man->maxfrees = 0;		/* 用于观察可用信息状况，frees最大值 */
	man->lostsize = 0;		/* 释放失败的内存总和 */
	man->losts = 0;			/* 释放失败次数 */
	return;
}

/////////////////////////////////////////////////////////////////////////////////////
//功能：获取空余内存的时际大小
//参数：
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

/////////////////////////////////////////////////////////////////////////////////////
//功能：分配内存
//参数：大小
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			//找到了足够大的内存
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* free[i]变成了0就减掉一条信息 */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* 带入结构体 */
				}
			}
			return a;
		}
	}
	return 0; /* 没有可用空间 */
}

/////////////////////////////////////////////////////////////////////////////////////
//功能：内存释放
//参数：
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	/* 为了方便归纳内存，将free[]按照addr的顺序排列 */
	/* 所以，现决定应该放到哪里 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前面有可用内存 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 可以与前面的可用内存归纳到一起 */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 后面也有 */
				if (addr + size == man->free[i].addr) {
					/* 也可与后面可用内存归纳到一起 */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]删除 */
					/* free[i]变成0后归纳到前面去 */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* 结构体赋值 */
					}
				}
			}
			return 0; /* 成功完成 */
		}
	}
	/* 不能与前面的可用空间归纳到一起 */
	if (i < man->frees) {
		/* 后面有 */
		if (addr + size == man->free[i].addr) {
			/* 可以与后面的内容归纳到一起 */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功完成 */
		}
	}
	/* 既不能与前面的内容归纳到一起，也不能与后面的内容归纳到一起，没有剩余的空间可以分配了 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]之后的，向后移动，腾出一点可用空间 */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* 更新最大值 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功完成 */
	}
	/* 不能后移 */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失败 */
}

/////////////////////////////////////////////////////////////////////////////////////
//功能：
//参数：
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

/////////////////////////////////////////////////////////////////////////////////////
//功能：
//参数：
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
