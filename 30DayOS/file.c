
#include "bootpack.h"

/////////////////////////////////////////////////////////////////////////////////////
//功能：将磁盘映像中的FAT解压缩
//参数：展开到fat[],img为磁盘地址
//附加：如果按照16b的方法type肯定可以正确显示文件开头的512字节的内容，如果遇到大于512字节的文件，可能就会显示出其他文件的内容
//按照windows管理磁盘的方法，保存大于512字节的文件时，有时并不是存入连续的扇区中，虽然这种情况不多~
//其实对于文件下一段存放在哪里，在磁盘里是有记录的，我们只要分析这个记录就能正确读取文件内容了，这个记录称为FAT"file allocation table"
//文件分配表，补充一下，在磁盘中没有存放在连续的扇区，就称为磁盘碎片
//但是FAT中的是直接不可看懂的，要解压：F0 FF FF -> FF0 FFF
//其实这种记录方法如果一旦坏了，就会混乱，为此磁盘中放了2个FAT,第一个位于0x000200~0x0013ff;第二个位于0x001400~0x0025ff
void file_readfat(int *fat, unsigned char *img)
{
	int i, j = 0;
	for (i = 0; i < 2880; i += 2) {
		fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;
		fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
		j += 3;
	}
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：将文件的内容读入内存，这样文件的内容已经排列为正确的顺序
//参数：
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
	int i;
	for (;;) {
		if (size <= 512) {//读取内存直接读好退出
			for (i = 0; i < size; i++) {
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for (i = 0; i < 512; i++) {//当大于512时就分别读
			buf[i] = img[clustno * 512 + i];
		}
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}
/////////////////////////////////////////////////////////////////////////////////////
//功能：寻找文件
//参数：
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
	int i, j;
	char s[12];
	for (j = 0; j < 11; j++) {
		s[j] = ' ';
	}//初始化s[]让其为空格
	j = 0;
	for (i = 0; name[i] != 0; i++) {
		if (j >= 11) { return 0; /* 没有找到 */ }
		if (name[i] == '.' && j <= 8) {
			j = 8;
		} else {
			s[j] = name[i];
			if ('a' <= s[j] && s[j] <= 'z') {
				/* 转换为小写 */
				s[j] -= 0x20;
			} 
			j++;
		}
	}//将名字转换为12字节的8+3名字+扩展名的存储模式,便于比较
	for (i = 0; i < max; ) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}//不存在文件直接退出
		if ((finfo[i].type & 0x18) == 0) {
			for (j = 0; j < 11; j++) {
				if (finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			return finfo + i; /* 找到返回 */
		}
next:
		i++;
	}
	return 0; /* 没有找到 */
}
