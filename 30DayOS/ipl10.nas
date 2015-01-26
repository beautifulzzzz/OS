; haribote-ipl
; TAB=4
CYLS    EQU        10                ; 定义读的柱面数
        ORG        0x7c00            ; 指明程序装载地址

; 以下这段是FAT12格式软盘专用代码  0x7c00--0x7dff 125字节 用于启动区
        JMP        entry
        DB        0x90
        DB        "HARIBOTE"         ; 启动区的名字可以是任意的，但必须是8字节
        DW        512                ; 每个扇区(sector)的大小必须为512字节
        DB        1                  ; 簇(cluster)的大小必须为1个扇区
        DW        1                  ; FAT的起始位置(一般从第一个扇区开始)
        DB        2                  ; FAT的个数(必须为2)
        DW        224                ; 根目录的大小(一般设为244项)
        DW        2880               ; 该磁盘的的大小(必须为2880扇区)
        DB        0xf0               ; 磁盘的种类(必须为0xfd)
        DW        9                  ; FAT的长度(必须为9扇区)
        DW        18                 ; 一个磁道(track)有几个扇区(必须为18)
        DW        2                  ; 磁头数(必须为2)
        DD        0                  ; 不使用分区(必须为0)
        DD        2880               ; 重写一次磁盘大小
        DB        0,0,0x29           ; 意义不明，固定
        DD        0xffffffff         ; (可能是)卷标号码
        DB        "HARIBOTEOS "      ; 磁盘名称(11字节)
        DB        "FAT12   "         ; 磁盘格式名称(8字节)
        RESB    18                   ; 先腾出18字节

; 程序核心
entry:
        MOV        AX,0              ; 初始化寄存器
        MOV        SS,AX
        MOV        SP,0x7c00
        MOV        DS,AX

; 读磁盘(从软盘中读数据装到内存中0x8200--0x83ff  125字节的地方)
        MOV        AX,0x0820         ; ES:BX=缓冲地址
        MOV        ES,AX
        MOV        CH,0              ; 柱面0
        MOV        DH,0              ; 磁头0
        MOV        CL,2              ; 扇区2   
readloop:        
        MOv     SI,0                 ; 记录失败次数的寄存器
retry:
        MOV        AH,0x02           ; AH=0x02 : 读盘
        MOV        AL,1              ; 1个扇区
        MOV        BX,0
        MOV        DL,0x00           ; A驱动器
        INT        0x13              ; 调用磁盘BIOS
        JNC     next                 ; 没出错就跳转到next继续读下一个做准备
        ADD     SI,1                 ; SI++
        CMP     SI,5                 ; 比较SI和5
        JAE     error                ; SI>=5时，跳转到error
        MOV     AH,0x00
        MOV     DL,0x00              ; A驱动器
        INT     0x13                 ; 重置驱动器
        JMP     retry           
next:   
        MOV     AX,ES                ; 把内存地址后移0x200
        ADD     AX,0x0020
        MOV     ES,AX                ; 因为没有ADD ES,0x200指令，所以这里绕个弯
        ADD     CL,1                 ; 往CL里加1 (所读扇区标号，开始是2，见初始化部分)
        CMP     CL,18                ; 和18比较 
        JBE     readloop             ; 小于18就跳转到readloop继续读 
        MOV     CL,1                 ; 扇区1
        ADD     DH,1                 ; 磁头+1
        CMP     DH,2                 ; 判断磁头是否超过2
        JB      readloop             ; 没有超过就继续读
        MOV     DH,0                 ; 超过2就转为0
        ADD     CH,1                 ; CH记录读取的柱面数
        CMP     CH,CYLS              ; CYLS在前面定义 CYLS EQU 10
        JB      readloop

; 磁盘内容装载内容的结束地址告诉haribote.sys
        MOV        [0x0ff0],CH       ; 将CYLS的值写到内存地址0x0ff0中 
        JMP        0xc200        

error:
        MOV        SI,msg            ; 循环输出msg里面的内容     
        
putloop:
        MOV        AL,[SI]
        ADD        SI,1              ; 给SI加1
        CMP        AL,0
        JE        fin
        MOV        AH,0x0e           ; 显示一个文字
        MOV        BX,15             ; 指定字符颜色
        INT        0x10              ; 调用显卡BIOS
        JMP        putloop                       
fin:
        HLT                          ; 让CPU停止等待指令
        JMP        fin               ; 无限循环
        
msg:
        DB        0x0a, 0x0a         ; 换行2次
        DB        "load error"
        DB        0x0a               ; 换行
        DB        0

        RESB    0x7dfe-$             ; 0x7dfe傑偱傪0x00偱杽傔傞柦椷

        DB        0x55, 0xaa
