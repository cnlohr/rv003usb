ENTRY( Scratchpad )

MEMORY
{
	SCRATCHPAD (rwx) : ORIGIN = 0x20000400, LENGTH = 128
	MEMORY (rwx) : ORIGIN = 0x20000480, LENGTH = 512
}

SECTIONS
{
	.init :
	{
		LONG( 0x000000aa )
		_sinit = .;
		. = ALIGN(4);
		KEEP(*(SORT_NONE(.init)))
		. = ALIGN(4);
		_einit = .;

		. = ALIGN(4);
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata*)
		*(.gnu.linkonce.t.*)
		. = ALIGN(4);

		KEEP(*(SORT_NONE(.fini)))
		. = ALIGN(4);

		PROVIDE( _etext = . );
		PROVIDE( _eitcm = . );  

		. = ALIGN(4);
		*(.gnu.linkonce.r.*)
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		. = ALIGN(8);
		PROVIDE( __global_pointer$ = . + 0x800 );
		*(.sdata .sdata.*)
		*(.sdata2*)
		*(.gnu.linkonce.s.*)
		. = ALIGN(8);
		*(.srodata.cst16)
		*(.srodata.cst8)
		*(.srodata.cst4)
		*(.srodata.cst2)
		*(.srodata .srodata.*)
		. = ALIGN(4);
		PROVIDE( _edata = .);


	} >SCRATCHPAD AT>SCRATCHPAD

	.memory :
	{
		. = ALIGN(4);
		PROVIDE( _sbss = .);
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss*)
		*(.gnu.linkonce.b.*)    
		*(COMMON*)
		. = ALIGN(4);
		PROVIDE( _ebss = .);

	} > MEMORY AT>MEMORY
	

    PROVIDE( _end = _ebss);
	PROVIDE( end = . );

	PROVIDE( _eusrstack = ORIGIN(RAM) + LENGTH(RAM));	
}

