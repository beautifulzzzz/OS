OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
		int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj \
		mtask.obj window.obj console.obj file.obj 


TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

# 设置默认命令为产生img
default :
	$(MAKE) img

# 正常执行过程
# 将ipl10.nas编译为ipl10.bin文件
ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl10.lst

# 将asmhead.asm编译为asmhead.bin
asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

# 将字库hankaku.txt编译为hankaku.bin
hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

# 将字库转换为.obj
hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

# 将OBJS_BOOTPACK的obj编译为.bim映像文件，这里的前提条件是有OBJS_BOOTPACK，所以会自动执行把naskfunc.nas和其它.c文件编译为obj文件
bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

# 将bootpack.bim映像编译为.hrb文件
bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

# 将.hrb映像和asmhead.bin 合并做成haribote.sys系统文件
haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb haribote.sys
	
# 将应用程序hello.nas转化为hrb文件
hello.hrb : hello.nas Makefile
	$(NASK) hello.nas hello.hrb hello.lst
hello2.hrb : hello2.nas Makefile
	$(NASK) hello2.nas hello2.hrb hello2.lst

# 将应用程序hello3生成
hello3.bim : hello3.obj a_nask.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello3.bim map:hello3.map hello3.obj a_nask.obj
hello3.hrb : hello3.bim Makefile
	$(BIM2HRB) hello3.bim hello3.hrb 0

# 把ipl10.bin和haribote.sys结合做成操作系统文件haribote.img
# 将ipl10.nas  make.bat hlt.hrb 这3个文件也读进去
haribote.img : ipl10.bin haribote.sys Makefile \
		hello.hrb hello2.hrb  hello3.hrb
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
		copy from:ipl10.nas to:@: \
		copy from:make.bat to:@: \
		copy from:hello.hrb to:@: \
		copy from:hello2.hrb to:@: \
		copy from:hello3.hrb to:@: \
		imgout:haribote.img

# 对c文件进行编译
# 把.c文件编译为.gas文件，需要.c bootpack.h 和 Makefile文件，然后调用cc1.exe进行处理
%.gas : %.c bootpack.h Makefile
	$(CC1) -o $*.gas $*.c

# 把.gas文件转换为.nas文件，其他类似上面的说明
%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

# 把.nas转为.obj文件并输出.lst文件
%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

# 根据不同的命令分别执行下面的情况，默认为img
img :
	$(MAKE) haribote.img

run :
	$(MAKE) img
	$(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: haribote.img

clean :
	$(DEL) *.bin
	$(DEL) *.lst
	$(DEL) *.obj
	$(DEL) *.bak
	$(DEL) *.map
	$(DEL) *.bim
	$(DEL) *.hrb
	$(DEL) *.sys

src_only :
	$(MAKE) clean
	$(DEL) haribote.img
	$(DEL) *.hrb