OUTPUT_FORMAT ("elf32-littlearm","elf32-bigarm","elf32-littlearm")

/* Setting code entry point to the linker */
ENTRY(_startup)


/* Memory Map*/
MEMORY
{
	sram (rwx) : ORIGIN = 0xFFFD0000, LENGTH = 0x00010000  /* assign 64KB of the internal SRAM for code and data */
}

/* Section Definitions */
SECTIONS
{
	/* section_name start_address BLOCK(align) (NOLOAD) : AT ( load_addr )
	{
		file_name (section_name, section_name, section_name)
		file_name (section_name, section_name, section_name)
	} > mem_area
	
	file_name:  '*'  means any file.
	section_name: '*'  means any section.
	start_address: can be "." to continue from last section. when not used at all it use the "> mem_area"
	*/

    text_startup :
    {
        *startup*(.text,  .text.*)
        *startup*(.rodata,  .rodata*)
    } > sram /* place start up code here, typically at ROM area. */
       
    text_all_others :
    {
        *(.text  .text.*)	/* all .text sections (the code) */
    } > sram /* place all other code here, typically at ROM area. */
    
    rodata :
    {
        *(.rodata  .rodata*)         /* all .rodata sections (RO data, constants, strings)  */
        *(.glue_7t  .glue_7)         /*  ARMv7 core glue functions for thumb and ARM mode */
		*(.gnu.linkonce.r.*  .gnu.linkonce.t.*)
    } > sram /* Read only data, typically at ROM area. */
    
    ARM.extab :
    {
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)
    } > sram /* ROM area */

    ARM.exidx :
    {
		__exidx_start = .;
		*(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
		__exidx_end = .;
    } > sram /* ROM area */
     
    . = ALIGN(4);
    
    _etext = ABSOLUTE(.);
    _sidata = ABSOLUTE(.);

    /* .data section which is used for initialized data (global variable). The init values are stored at ROM (_etext), at 'strat.s' it is copied to RAM */
    data . : AT (_etext)   /* the RO data is placed at _etext (the load_addr) while the RW data will be copied into _sdata*/
    {
        . = ALIGN(4);
        _sdata = ABSOLUTE(.);
        ASSERT(_sidata == _sdata, "Error: in RAM compilation, _sidata and _sdata should be equal.");
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = ABSOLUTE(.) ;
    } > sram /* RAM area */


    /* .bss section which is used for uninitialized data (global variable); no pre-load is needed, just place holder in RAM */
    bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = ABSOLUTE(.);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = ABSOLUTE(.);
    } > sram  /* RAM area */


    stack (NOLOAD) :
    {
        . = ALIGN(4);
        __StackLimit = ABSOLUTE(.);  	    /* may use to monitor the bottom point of the stack */
        __StackSize = 0x1000; /* LENGTH (sram) - SIZEOF (startup); */
        ASSERT(__StackSize >= 0x1000, "Error: Stack size is less then the minimume required.");
        . = . + __StackSize;
        __StackTop = ABSOLUTE(.);         /* may use to set top point of the stack */
    } > sram /* ram */
    
 
    . = ALIGN(4);
    end = ABSOLUTE(.) ;  		    /* end of assigned RAM, may use to set start point of the heap (used by libc or librdimon.a in rdimon-syscalls.o - sbrk()) */

}
