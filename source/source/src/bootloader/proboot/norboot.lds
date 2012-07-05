OUTPUT_ARCH(arm)
SECTIONS
{
	_nor_offset = 0x0000;
	. = 0x19800000 + _nor_offset; /* nor flash address (+64k) */
	/* . = 0x00800000; /* test in MEM */

	_fbootsect = .;
	.boot : {
		LONG(0x10291029)
		bootsect.o(.text)
		. = ALIGN(4);
		_fmarch = .;
		diagnostic/march.o
		_emarch = .;
		. = ALIGN(16);
		LONG(0)
		LONG(0)
		LONG(0)
		LONG(0)
	}
	_ebootsect = .;
	

	. = 0x00600000; /* original proboot RAM address (for proboot shadow) */
	_fproboot = .;
	.text : {
		init.o(.text)
		* (.text)
		* (.rodata)
		* (.rodata.str1.4)

		. = ALIGN(4);
		__cmd_start = .;
		* (.cmd)
		__cmd_end = .;

		. = ALIGN(4);
		__fscript = .;
		script.o(.data)
		__escript = .;

		. = ALIGN(4);
		__fhttp_page = .;
		http_page.o(.data)
		__ehttp_page = .;
		
		. = ALIGN(4);
		__fhttp_progress = .;
		http_progress.o(.data)
		__ehttp_progress = .;
		
		. = ALIGN(4);
		__fhttp_page2 = .;
		http_page2.o(.data)
		__ehttp_page2 = .;
		
		. = ALIGN(4);

	}
	_eproboot = .;

	/*. = 0x00600000; /* original proboot RAM address (for globol variable) */

	_fdata = ALIGN(4);
	.data : {

		* (.data)
	}
	_edata =.;

	_sbss = ALIGN(4);
	.bss :
	{
		*(.bss)
		*(.common)
		. += 4;
	}
	_ebss = .;


}
