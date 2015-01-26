; haribote-os boot asm
; TAB=4

[INSTRSET "i486p"]  ;必须加！！！

VBEMODE	EQU		0x103	; 1024 x  768 x 8bit  //显示模式
BOTPAK    EQU        0x00280000        ; bootpack的内存首址
DSKCAC    EQU        0x00100000        ; 
DSKCAC0    EQU        0x00008000        ; 

; BOOT_INFO相关
CYLS    EQU        0x0ff0            ; 设定启动区
LEDS    EQU        0x0ff1
VMODE    EQU        0x0ff2            ; 关于颜色的信息颜色的位数
SCRNX    EQU        0x0ff4            ; 分辨率X
SCRNY    EQU        0x0ff6            ; 分辨率Y
VRAM    EQU        0x0ff8            ; 图像缓冲区的开始地址

        ORG        0xc200            ; 这个程序要装在到什么位置,在ipl10读盘结束的地方转到该处
;-----------------------------------------------画面模式设置
;确认VBE是否存在[显示模式协议BOOS]
;给AX,ES,DI赋好值之后调用int如果AX没有变成0x004f就没有VBE就只能用320*200的分辨率了
;JMP   		scrn320  ;便于调试我们直接还是用老的显示模式，所以直接跳过高分辨率的部分
		MOV		AX,0x9000
		MOV		ES,AX
		MOV		DI,0
		MOV		AX,0x4f00
		INT		0x10
		CMP		AX,0x004f
		JNE		scrn320
;检查VBE的版本
		MOV		AX,[ES:DI+4]
		CMP		AX,0x0200
		JB		scrn320			; if (AX < 0x0200) goto scrn320		
;取得画面模式
		MOV		CX,VBEMODE
		MOV		AX,0x4f01
		INT		0x10
		CMP		AX,0x004f
		JNE		scrn320
;画面模式信息确认
		CMP		BYTE [ES:DI+0x19],8
		JNE		scrn320
		CMP		BYTE [ES:DI+0x1b],4
		JNE		scrn320
		MOV		AX,[ES:DI+0x00]
		AND		AX,0x0080
		JZ		scrn320	
;画面模式切换
		MOV		BX,VBEMODE+0x4000
		MOV		AX,0x4f02
		INT		0x10
		MOV		BYTE [VMODE],8	; 记下画面模式
		MOV		AX,[ES:DI+0x12]
		MOV		[SCRNX],AX
		MOV		AX,[ES:DI+0x14]
		MOV		[SCRNY],AX
		MOV		EAX,[ES:DI+0x28]
		MOV		[VRAM],EAX
		JMP		keystatus		
scrn320:
		MOV		AL,0x13			; VGA图、320x200x8bit彩色,如果高分辨率模式不行就只能用低分辨率模式了
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 记下画面模式
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000
		
;高分辨率{给AX赋值0x4f02,BX赋值模式码{这里要把模式码+0x4000,QEMU中不能直接用下面的方法}
;0x101  640 480 8bit
;0x103  800 600 8bit
;0x105  1024 768 8bit
;0x107  1280 1024 8bit
;				MOV					BX,0x4101
;				MOV					AX,0x4f02
;				INT					0x10
;				MOV					BYTE[VMODE],8
;				MOV         WORD[SCRNX],640
;				MOV					WORD[SCRNY],480
;				MOV					DWORD[VRAM],0xe0000000
;-----------------------------------------------画面模式设置				
; 用BIOS获得键盘上各种LED指示灯的状态
keystatus:
        MOV        AH,0x02
        INT        0x16             ; keyboard BIOS
        MOV        [LEDS],AL

; PIC关闭一切中断
;        根据AT兼容机的规格，如果要初始化PIC(Programmable Interrupt Controller)
;        必须在CLI之前进行，否则有时会挂起
;        随后进行PIC的初始化
;   这段等价于：
;            io_out8(PIC0_IMR,0xff);禁止主PIC的中断
;            io_out8(PIC1_IMR,0xff);禁止从PIC的中断
;            io_cli();禁止CPU级中断

        MOV        AL,0xff
        OUT        0x21,AL
        NOP                        ; 如果连续执行OUT指令有些机种会无法执行
        OUT        0xa1,AL

        CLI                        ; 禁止CPU级别中断

; 为了让CPU访问1MB以上的内存空间，设定A20GATE
; 相当于
;        #define KEYCMD_WRITE_OUTPORT 0xd1
;        #define KBC_OUTPORT_A20G_ENABLE 0xdf
;        wait_KBC_sendready();
;        io_out8(PORT_KEYCMD,KEYCMD_WRITE_OUTPORT);
;        wait_KBC_sendready();
;        io_out8(PORT_KEYDAT,KBC_OUTPORT_A20G_ENABLE);
;        wait_KBC_sendready();
;    该程序和init_keyboard完全相同，仅仅是向键盘控制电路发送指令，这里发送的指令，是指令键盘控制电路的附属端口输出0xdf
; 这个附属端口，连接着主板上的很多地方，通过这个端口发送不同的命令，就可以实现各种各样的控制功能了
;    这次输出所要完成的功能，是让A20GATE信号变成ON状态，是让1MB以上的内存变成可用状态

        CALL    waitkbdout
        MOV        AL,0xd1
        OUT        0x64,AL
        CALL    waitkbdout
        MOV        AL,0xdf            ; enable A20
        OUT        0x60,AL
        CALL    waitkbdout

; 切换保护模式
; INSTRSET指令是为了386以后的LGDT,EAX,CR0
; LGDT指令把随意准备的GDT读出来，对于这个暂定的GDT我们今后还要重新设定，然后将CR0这一特殊的32位寄存器的值带入EAX
;    并将高位置0，低位置1，再将这个值返回给CR0,这就完成了模式转化，进入到不用颁的保护模式。
; CR0是control register 0是一个非常重要的寄存器,只有操作系统才能操作它
; protected virtual address mode 受保护的虚拟内存地址模式，在该模式下，应用程序不能随便改变段的设定，又不能使用操作系统的段。操作系统受CPU保护
; 在保护模式中主要有受保护的16位模式和受保护的32位模式，我们要使用的是受保护的32位模式
; 在讲解CPU的书上会写到，通过带入CR0而切换到保护模式时，马上就执行JMP指令。所以我们也执行这一指令。为什么要jmp呢？因为变成保护模式后，机器语言的解释发生变化，
; CPU为了加快指令的执行速度而使用了管道(pipeline)这一机制，也就是说前一条指令还在执行时，就开始解释下一条甚至是再下一条指令，因为模式变了就要重新解释一遍，so..
; 而且在程序中，进入保护模式后，段寄存器的意思也变了(不是*16再加算的意思)，除了CS以外所有的段寄存器的值都从0x0000变成了0x0008.CS保持不变是因为CS如果变了就乱了
; 所以只有CS要放到后面处理。这里的0x0008相当于gdt+1的段

[INSTRSET "i486p"]                ; ”想要使用486指令"的叙述

        LGDT    [GDTR0]            ; 设定临时的GDT(Global Descriptor Table)
        MOV        EAX,CR0
        AND        EAX,0x7fffffff    ; 设bit31为0(为了禁止颁)
        OR        EAX,0x00000001    ; 设bit0为1(为了切换保护模式)
        MOV        CR0,EAX
        JMP        pipelineflush
pipelineflush:
        MOV        AX,1*8            ;  可读写的段32bit
        MOV        DS,AX
        MOV        ES,AX
        MOV        FS,AX
        MOV        GS,AX
        MOV        SS,AX

; bootpack传送
; 简单来说，这部分只是在调用memcpy函数(大概意思是：下面表达仅为了说明意思，可能不正确)
; memcpy(转送源地址    ,转送目的地址,转送数据大小);大小用双字，所以用字节/4、
; memcpy(bootpack        ,BOTPAK            ,512*1024/4                        );//从bootpack的地址开始的512kb内容复制到0x00280000号地去[512kb比bootpack.hrb大很多]
;    memcpy(0x7c00            ,DSKCAC            ,512/4                                );//从0x7c00复制512字节到0x10000000是将启动扇区复制到1MB以后的内存去，[启动区的0x00007c00-0x00007dff，125字节]
;    memcpy(DSKCAC+512    ,DSKCAC+512    ,cyls*512*18*2/4-512/4);//从始于磁盘0x00008200的内容，复制到0x00100200内存去[从磁盘读取数据装到0x8200后的地方]
; bootpack是asmhead的最后一个标签，因为.sys是通过asmhead.bin和bootpack.hrb连接而成，所以asmhead结束的地方就是bootpack.hrb最开始地方
        MOV        ESI,bootpack    ; 转送源
        MOV        EDI,BOTPAK      ; 转送目的地
        MOV        ECX,512*1024/4
        CALL    memcpy

; 磁盘的数据最终转送到它本来的位置去

; 首先从启动扇区开始

        MOV        ESI,0x7c00        ; 转送源
        MOV        EDI,DSKCAC        ; 转送目的地
        MOV        ECX,512/4
        CALL    memcpy

; 所有剩下的

        MOV        ESI,DSKCAC0+512    ; 转送源
        MOV        EDI,DSKCAC+512    ; 转送目的地
        MOV        ECX,0
        MOV        CL,BYTE [CYLS]
        IMUL    ECX,512*18*2/4    ; 从柱面数转为字节数/4
        SUB        ECX,512/4        ; 减去IPL
        CALL    memcpy

; 必须由asmhead来完成，至此全部完毕
; 以后就交给bootpack来完成

; bootpack的启动
; 还是执行memecpy程序，将bootpack.hrb第0x10c8字节开始的0x11a8字节复制到0x00310000号地址去
; 最后将0x310000代入到ESP中，然后用一个特殊的JMP指令，将2*8代入到CS里，同时移动到0x1b号地址，这里的0x1b是指第二个段的0x1b号地址
; 第2个段的基址是0x280000,所以是从0x28001b开始执行，即bootpack.hrb的0x1b号地址
        MOV        EBX,BOTPAK
        MOV        ECX,[EBX+16]
        ADD        ECX,3            ; ECX += 3;
        SHR        ECX,2            ; ECX /= 4;
        JZ        skip            ; 没有要转送的东西时
        MOV        ESI,[EBX+20]    ; 转送源
        ADD        ESI,EBX
        MOV        EDI,[EBX+12]    ; 转送目的地
        CALL    memcpy
skip:
        MOV        ESP,[EBX+12]    ; 栈初始值
        JMP        DWORD 2*8:0x0000001b
;----------------------------------------------------------------------------------------------------
;内存分配：
; 0x00000000-0x000fffff:虽然在启动中会多次使用，但之后就会变空(1M)
; 0x00100000-0x00267fff:用于保存软盘的内容(1440KB)
;    0x00268000-0x0026f7ff:空(30KB)
; 0x0026f800-0x0026ffff:IDT(2KB)
; 0x00270000-0x0027ffff:GDT(64KB)
; 0x00280000-0x002fffff:bootpack.hrb(512KB)
; 0x00300000-0x003fffff:栈及其他(1MB)
; 0x00400000-                     :空
;----------------------------------------------------------------------------------------------------

; 和wait_KBC_sendready相同，但加入了0x60号设备进行IN处理，也就是如果控制器里有键盘代码，或已经积累了鼠标数据，就顺便读出来
waitkbdout:
        IN         AL,0x64
        AND         AL,0x02
        JNZ        waitkbdout        ; AND偺寢壥偑0偱側偗傟偽waitkbdout傊
        RET

; 复制程序
memcpy:
        MOV        EAX,[ESI]
        ADD        ESI,4
        MOV        [EDI],EAX
        ADD        EDI,4
        SUB        ECX,1
        JNZ        memcpy            ; 减法运算的结果如果不是0，就跳转到memcpy
        RET

; GDT0也是一种特定的GDT,0是空区域(null sector),不能在那里定义段，1号和2号分别由下式设定：
; set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);//设定全局段
; set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);//设定bootpack512字节的段

        ALIGNB    16;一直添加DBO，直到地址能被16整除，如果GDT0地址不是8的倍数就会慢一些，所以...
GDT0:                
        RESB    8                ; NULL selector
        DW        0xffff,0x0000,0x9200,0x00cf    ; 可以读写的段(segment) 32bit
        DW        0xffff,0x0000,0x9a28,0x0047    ; 可执行的段(segment) 32bit (bootpack用)

        DW        0
GDTR0:            ;是LGDT指令，通知GDT0有了GDT,在GDT0写入了16位的段上限和32位段其实地址
        DW        8*3-1
        DD        GDT0

        ALIGNB    16
bootpack:

; 也就是说，最初的状态时，GDT在asmhead.nas里并不在0x00270000-0x0027ffff的范围里。IDT连设定都没设定，所以仍处于中断禁止状态
; 应当趁着硬件上积累过多的数据而产生的错误之前，尽快开放中断，接受数据。因此，在bootpack.c的hariMain里，应该在进行调色板的
; 初始化之前及画面准备之前，赶紧重新建立GDT和IDT,初始化PIC,并执行io_sti();
