
.extern main
.text
.global _start
_start:

	b Reset

 @ 0x04: 未定义指令中止模式的向量地址
HandleUndef:
    b   HandleUndef 
 
@ 0x08: 管理模式的向量地址，通过SWI指令进入此模式
HandleSWI:
    b   HandleSWI

@ 0x0c: 指令预取终止导致的异常的向量地址
HandlePrefetchAbort:
    b   HandlePrefetchAbort

@ 0x10: 数据访问终止导致的异常的向量地址
HandleDataAbort:
    b   HandleDataAbort

@ 0x14: 保留
HandleNotUsed:
    b   HandleNotUsed

@ 0x18: 中断模式的向量地址
    b   HandleIRQ

@ 0x1c: 快中断模式的向量地址
HandleFIQ:
    b   HandleFIQ

Reset:
		
	ldr sp, =4096 @ 设置栈指针
	bl disable_watch_dog
	bl clock_init
	bl memsetup
	bl nand_init

	ldr r0, =0x30000000
	mov r1, #0
	ldr r2, = __bss_start
	sub r2,r2,r0
	bl CopyCode2SDRAM

	bl clean_bss
	msr cpsr_c, #0xd2
	ldr sp, =0x31000000

	  msr cpsr_c, #0xdf       @ 进入系统模式    
	  ldr sp, =0x34000000     @ 设置系统模式栈指针，

	ldr lr, =ret_initirq
	ldr pc, =init_irq
ret_initirq:
	msr cpsr_c, #0x5f

	ldr lr, =halt_loop
	ldr pc, =main
halt_loop:
	b halt_loop
HandleIRQ:
	sub lr, lr, #4
	stmdb sp!, {r0-r12,lr}

	ldr lr, =int_return
	ldr pc, =IRQ_Handle
int_return:
	ldmia sp!, {r0-r12,pc}^

