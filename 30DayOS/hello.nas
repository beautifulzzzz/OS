;---------------------------------------
;2、用汇编写的应用程序
;---------------------------------------
[INSTRSET "i486p"]
[BITS 32]

		MOV		ECX,msg
		MOV		EDX,1
putloop:
		MOV		AL,[CS:ECX]
		CMP		AL,0
		JE		fin
		INT		0x40
		ADD		ECX,1
		JMP		putloop
fin:
		RETF
msg:
		DB		"hello",0

;---------------------------------------
;1、简单的hello输出
;---------------------------------------
;[BITS 32]
;	MOV		AL,'H'
;	INT		0x40 		;CALL	2*8:0xbea 0x00000BEA:_asm_cons_putchar，在dsctbl设置为类似BIOS调用类似的API了
;	MOV		AL,'e'
;	INT		0x40
;	MOV		AL,'l'
;	INT		0x40
;	MOV		AL,'l'
;	INT		0x40
;	MOV		AL,'o'
;	INT		0x40
;	RETF
;--------------------------------------
