
/*-----------------------------------------------------------------------------
@ Macro name: m_close_mapping
@ Description: Set 0x1C000004[2]=0 to *DISABLE* hardware map 0x00000000 to 0x1FC00000
@
-----------------------------------------------------------------------------*/
.macro m_close_mapping
	ldr r0,=Reg_2
	ldr r1,[r0]
	bic r1,r1,#0x04
	str r1,[r0]
.endm

/*-----------------------------------------------------------------------------
@ Macro name: m_check_nor_flash
@ Description:  check if NOR Flash exist ?
@		if 0x19800000[31:0]=0x10291029,Nor Flash exist, jump to 0x19800004
@		else , Nor Flash does not exist
-----------------------------------------------------------------------------*/
.macro m_check_nor_flash
	ldr r0,=NOR_DIR_ADDRESS
	ldr r1,[r0]
	ldr r2,=0x10291029
	cmp r1,r2
	bne not_nor_flash
	m_show_status 0x2F
	add pc,r0,#4
not_nor_flash:		
.endm



/*-----------------------------------------------------------------------------
@ Macro name: write_memory address, data
@ Description: write 32-bits data to target address
-----------------------------------------------------------------------------*/
.macro write_memory address, data
	ldr r1, =\address
	ldr r2, =\data
	str r2,[r1]
.endm

/*-----------------------------------------------------------------------------
@ Macro name: read_memory address,reg=r2
@ Description: read 32-bits data from address to reg
-----------------------------------------------------------------------------*/
.macro read_memory address,reg=r2
	ldr r1,=\address
	ldr \reg,[r1]
.endm

/*-----------------------------------------------------------------------------
@ Macro name: m_memory_sizing
@ Description: memory_part_A..E
@
-----------------------------------------------------------------------------*/
.macro m_memory_sizing
	mov r7,#0
	
@ part_A the width of the memory data bus
mem_part_A:
#A-1
	write_memory 0x1C000000,0x10001AFF
#A-2
	write_memory 0x1C000004,0x08CC0B00
#A-3
	write_memory 0x1C000008,0x30802000
#A-4
	write_memory 0x1C00000C,0x00FF0032
#A-5	set 0x1B000018[19]=1
	ldr r1,=0x1B000018
	ldr r2,[r1]
	ldr r3,=0x80000
	orr r2,r2,r3
	str r2,[r1]
#A-6	set 0x1B000008[15:0]=0xFF
	ldr r1,=0x1B000008
	mov r2,#0xFF
	strh r2,[r1]	
#A-7
	write_memory 0x00000000,0x11111111
#A-8
	write_memory 0x00000004,0xaa55aa55
#A-9
	read_memory  0x00000000,r2
	ldr r3,=0x11111111
	cmp   r2,r3
  	orrne r7,r7,#0x02
exit_A:


@part_B the number of the Column Address bits
mem_part_B:
#B-1
	write_memory 0x00001000,0x10101010
#B-2
	write_memory 0x00000800,0x09090909
#B-3
	write_memory 0x00000400,0x08080808
#B-4
	write_memory 0x00000200,0x07070707
#B-5
	read_memory  0x00000000,r2
	ldr r3,=0x0000FFFF
	and r2,r2,r3

check_ca_11:
	ldr r3,=0x1111
	cmp r2,r3
	orreq r7,r7,#0x300
	beq   exit_B

check_ca_10:
	ldr r3,=0x1010
	cmp r2,r3
	orreq r7,r7,#0x200
	beq   exit_B

check_ca_9:
	ldr r3,=0x0909
	cmp r2,r3
	orreq r7,r7,#0x100
	beq   exit_B

check_ca_8:
	ldr r3,=0x0808
	cmp r2,r3
	beq exit_B

check_ca_error:
	m_show_status ERR_MCAS
	b check_ca_error

exit_B:


@part_C the number of the Row Address bits
mem_part_C:
#C-1
	write_memory 0x00008000,0x13131313
#C-2
  	write_memory 0x08008000,0x12121212
#C-3
  	write_memory 0x04008000,0x11111111
#C-4
  	read_memory 0x00008000,r2
  	ldr r3,=0x0000FFFF
	and r2,r2,r3

check_ra_13:
	ldr r3,=0x1313
	cmp r2,r3
	orreq r7,r7,#0x800
	beq exit_C

check_ra_12:
	ldr r3,=0x1212
	cmp r2,r3
	orreq r7,r7,#0x400
	beq exit_C

check_ra_11:
	ldr r3,=0x1111
	cmp r2,r3
	beq exit_C

check_ra_error:
	m_show_status ERR_MRAS
	b check_ra_error

exit_C:


@part_D the number of the internal bank
mem_part_D:
#D-1
	write_memory 0x00004000,0x02020202
#D-2
	read_memory 0x00000000,r2
	ldr r3,=0x0000FFFF
	and r2,r2,r3

check_ba_4:
	ldr r3,=0x0202
	cmp r2,r3
	bne exit_D			@ default, 4banks

check_ba_2:
	ldr r3,=0x2001
	orr r7,r7,r3		@ 2banks

exit_D:


@part_E to program the SDRAM Controller
mem_part_E:
#E-1
	ldr r1, =Reg_2			@ 0x1C000004
	str r7,[r1]
#E-2
	read_memory 0x1C000000,r2
	mov r2,r2,lsr #16

mem_256:
	cmp r2,#0x1000
	ldreq r3,=0x10FF
	beq mem_setting

mem_128:
	cmp r2,#0x0800	
	ldreq r3,=0x10EE
	beq mem_setting

mem_64:
	cmp r2,#0x0400	
	ldreq r3,=0x10DD
	beq mem_setting	

mem_32:
	cmp r2,#0x0200	
	ldreq r3,=0x10CC
	beq mem_setting

mem_16:
	cmp r2,#0x0100	
	ldreq r3,=0x10BB
	beq mem_setting

mem_8:
	cmp r2,#0x0080	
	ldreq r3,=0x10AA
	beq mem_setting
	
mem_4:
	cmp r2,#0x0040	
	ldreq r3,=0x1099
	beq mem_setting
	
mem_2:
	cmp r2,#0x0020	
	ldreq r3,=0x1088
	beq mem_setting
	
mem_error:
	m_show_status ERR_LESS_MEM
	b mem_error
	
mem_setting:	
	ldr r1, =0x1C000000
	str r3,[r1]
	
@part_F to set Mode-register again
mem_part_F:
#F-1	set 0x1B000018[19]=0
	ldr r1,=0x1B000018
	ldr r2,[r1]
	bic r2,r2,#1<<19
	str r2,[r1]
#F-2	set 0x1B000008[15:0]=0xFF
	ldr r1,=0x1B000008
	mov r2,#0xFF
	strh r2,[r1]	
.endm


