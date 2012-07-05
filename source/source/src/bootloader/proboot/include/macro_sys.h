/*-----------------------------------------------------------------------------
@ Macro name: m_disable_CPU_interrupt
@ Description:  Disable ARM IRQ & FIQ
-----------------------------------------------------------------------------*/
.macro m_disable_CPU_interrupt
	mrs r0,cpsr
	orr r0,r0,#0xC0
	msr cpsr,r0
.endm

/*-----------------------------------------------------------------------------
@ Macro name: m_disable_INTC_interrupt
@ Description:  set 0x1B000100=0 (Interrupt mask =0)
-----------------------------------------------------------------------------*/
.macro m_disable_INTC_interrupt
	ldr r0,=INT_EM_0
	ldr r1,=0
	strh r1,[r0]
.endm


/*-----------------------------------------------------------------------------
@ Macro name: m_set_clock
@ Description:  detect & set clock
-----------------------------------------------------------------------------*/
.macro m_set_clock
	ldr r1,=SYS_CLOCK
	ldr r2,[r1]

check_valid_bit:
	ands r3,r2,#1<<31 		@ bit 31
	bne no_clock			@ valid bit=1(zero flag=0), warm start, don't set clock

	mov r3, r2, lsr #24
	and r3,r3,#0x7			@ r3=bit 24-26

crystal_2MHz:
	cmp r3,#0
    	ldr r4,=0x10828884
	beq setting

crystal_12MHz:
	cmp r3,#2
    	ldr r4,=0x10828888
	beq setting

crystal_24MHz:
	cmp r3,#1
    	ldr r4,=0x10828890
	beq setting

outside_crystal:
    	b no_clock

setting:
    	str r4,[r1]			@ set clock

    	ldr r1,=SYS_CLOCK_CFG3
	ldrh r2,[r1]
	ldr  r3,=0xF0F
	bic  r2,r2,r3
	ldr  r3,=0x202
	orr  r2,r2,r3
	strh r2,[r1]			@ clock change write toggle 0x1B000014[11:8],[3:0]

    	ldr r1,=SYS_CLOCK_CHG
	strh r4,[r1]			@ clock change write toggle 0x1B000008[15:0]
no_clock:

.endm


/*-----------------------------------------------------------------------------
@ Macro name: m_setting_SP
@ Description:  set stack point
-----------------------------------------------------------------------------*/
.macro m_setting_SP data
	ldr sp, =\data
.endm


/*-----------------------------------------------------------------------------
@ Macro name: m_show_status status
@ Description:  output status to IO port 80
-----------------------------------------------------------------------------*/
.macro m_show_status status
	ldr r11,=def_DEBUG_PORT
	ldr r12,=\status
	strh r12,[r11]
.endm

