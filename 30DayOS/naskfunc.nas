; naskfunc
; TAB=4

[FORMAT "WCOFF"]                 ; 制成目标文件的形式   
[INSTRSET "i486p"]               ; 使用486格式命令
[BITS 32]                        ; 制作32模式用的机器语言
[FILE "naskfunc.nas"]            ; 源文件名信息
  ; 程序中包含的函数名
		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8,  _io_in16,  _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_load_cr0, _store_cr0				;新增
		GLOBAL	_load_tr
		GLOBAL	_asm_inthandler20,_asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
		GLOBAL	_memtest_sub								;新增
		GLOBAL  _farjmp;      							 代替GLOBAL	_taskswitch3, _taskswitch4
		EXTERN	_inthandler20 ,_inthandler21, _inthandler27, _inthandler2c

[SECTION .text]

_io_hlt:	; void io_hlt(void);
		HLT
		RET

_io_cli:	; void io_cli(void);
		CLI
		RET

_io_sti:	; void io_sti(void);
		STI
		RET

_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

_io_in8:	; int io_in8(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AL,DX
		RET

_io_in16:	; int io_in16(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AX,DX
		RET

_io_in32:	; int io_in32(int port);
		MOV		EDX,[ESP+4]		; port
		IN		EAX,DX
		RET

_io_out8:	; void io_out8(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		AL,[ESP+8]		; data
		OUT		DX,AL
		RET

_io_out16:	; void io_out16(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,AX
		RET

_io_out32:	; void io_out32(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,EAX
		RET

_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD		; PUSH EFLAGS 偲偄偆堄枴
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS 偲偄偆堄枴
		RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LGDT	[ESP+6]
		RET

;1) 这个函数用来指定的段上限和地址赋值给GDTR的48位寄存器，这是个很特殊的寄存器，并不能用MOV来直接赋值
;，唯一的方法就是指定一个内存地址，从指定的内存地址读取6字节（也就是48位），然后赋值给GDTR寄存器。完成这一任务的指令就是LGDT
;2) 该寄存器的低16位（即内存的最初的2个字节）是段上限，它等于“GDT的有效的字节数-1”，剩下的32位，代表GDT的开始地址
;3) 这里2个参数是ESP+4和ESP+8里存放，而我们要的是6字节形式的，所以要转换为我们想要的形式~
_load_idtr:        ; void load_idtr(int limit, int addr);
    MOV     AX,[ESP+4]        ; limit
    MOV     [ESP+6],AX
    LIDT    [ESP+6]
    RET

_load_cr0:		; int load_cr0(void);
		MOV		EAX,CR0
		RET

_store_cr0:		; void store_cr0(int cr0);
		MOV		EAX,[ESP+4]
		MOV		CR0,EAX
		RET
		
_load_tr:		; void load_tr(int tr);
		LTR		[ESP+4]			; tr
		RET

_asm_inthandler20:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD
		
;这个函数只是将寄存器的值保存在栈里，然后将DS和ES调整到与SS相等，再调用_inthandler21,返回后将所有寄存器的值再返回到原来的值，然后执行IRETD
;之所以如此小心翼翼地保护寄存器，原因在于，中断处理发生在函数处理途中，通过IREDT从中断处理后，寄存器就乱了
_asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV        EAX,ESP
    PUSH    EAX
    MOV        AX,SS
    MOV        DS,AX
    MOV        ES,AX
    CALL    _inthandler21
    POP        EAX
    POPAD
    POP        DS
    POP        ES
    IRETD

_asm_inthandler27:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV        EAX,ESP
    PUSH    EAX
    MOV        AX,SS
    MOV        DS,AX
    MOV        ES,AX
    CALL    _inthandler27
    POP        EAX
    POPAD
    POP        DS
    POP        ES
    IRETD

_asm_inthandler2c:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV        EAX,ESP
    PUSH    EAX
    MOV        AX,SS
    MOV        DS,AX
    MOV        ES,AX
    CALL    _inthandler2c
    POP        EAX
    POPAD
    POP        DS
    POP        ES
    IRETD


;/////////////////////////////////////////////////////////////////////////////////////
;//功能：检查区域内的内存是否有效,因为c编译器会清除掉一些东西，所以就只能用汇编写，见naskfunc.nas
;//参数：起始位置
;//附加：写入取反看看修改没，再取反看看修改没，如果两次都修改就认为是能够使用的内存
;unsigned int memtest_sub(unsigned int start,unsigned int end)
;{
;	unsigned int i,*p,old,pat0=0xaa55aa55,pat1=0x55aa55aa;
;	for(i=start;i<=end;i+=4){
;		p=(unsigned int *)i;
;		old=*p;
;		*p=pat0;
;		*p^=0xffffffff;
;		if(*p!=pat1){
;		not_memory:
;			*p=old;
;			break;			
;		}
;		*p^=0xffffffff;
;		if(*p!=pat0){
;			goto not_memory;
;		}
;		*p=old;
;	}
;	return i;
;}

_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH	EDI						; 由于还要使用EBX,ESI,EDI，所以对其进行保存
		PUSH	ESI
		PUSH	EBX
		MOV		ESI,0xaa55aa55			; pat0 = 0xaa55aa55;
		MOV		EDI,0x55aa55aa			; pat1 = 0x55aa55aa;
		MOV		EAX,[ESP+12+4]			; i = start;
mts_loop:
		MOV		EBX,EAX
		ADD		EBX,0xffc				; p = i + 0xffc;
		MOV		EDX,[EBX]				; old = *p;
		MOV		[EBX],ESI				; *p = pat0;
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		EDI,[EBX]				; if (*p != pat1) goto fin;
		JNE		mts_fin
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff;
		CMP		ESI,[EBX]				; if (*p != pat0) goto fin;
		JNE		mts_fin
		MOV		[EBX],EDX				; *p = old;
		ADD		EAX,0x1000				; i += 0x1000;
		CMP		EAX,[ESP+12+8]			; if (i <= end) goto mts_loop;
		JBE		mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
mts_fin:
		MOV		[EBX],EDX				; *p = old;
		POP		EBX
		POP		ESI
		POP		EDI
		RET

;用来代替下面的任务切换程序
_farjmp: 				; void farjmp(int eip,int cs);
		JMP 	FAR [ESP+4]   ;epi cs CPU从指定内存读4字节给EIP,再继续读2字节的数据，将其写入CS寄存器
		RET
;_taskswitch3:	; void taskswitch4(void);load_tr只是改变tr不进行任务切换，这里是far模式任务切换
;		JMP		3*8:0
;		RET
;_taskswitch4:	; void taskswitch4(void);load_tr只是改变tr不进行任务切换，这里是far模式任务切换
;		JMP		4*8:0
;		RET
