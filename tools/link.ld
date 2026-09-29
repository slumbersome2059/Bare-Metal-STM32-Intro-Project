/*the job of the linker script is to take the object files of 
the source files and integrate the parts of those files
into an ELF(executable and linkable file) format which has things
.text, .data, .heap, as sections ...

the elf file helps place program correctly in memory so
its used in the flashing process(this is where the information about
location and size of section comes in useful)
*/

/*
This is for debugger to know which function runs first 
so that it can set a breakpoint at the beginning of firmware.
*/
ENTRY(_reset); 


/*
Even though on datasheet it says first entry of vector table starts at 0x00000000
and we are mapping it to 0x08000000. The memory from 0x0 is an alias so when
something from that region is required eg 0x0 the hardware forwards request to address 
it is aliasing so in this case 0x08.
What the memory from 0x0 aliases to is determined by mode set on BOOT pins and 
by default it is flash but it can also be system memory or SRAM.
Only the first two interrupt handlers are aliased actually and not all because 
once these are accessed the program code will be read and executed and here 
it will set a VTOR register which will have offset of 0x08000000 and then when 
interrupt vectors are accessed this offset will be added so there is no aliasing for the other interrupts.
*/
MEMORY{
/*
The rx and rwx specify permissions.
*/
    flash(rx): ORIGIN = 0x08000000, LENGTH = 32k
    sram(rwx): ORIGIN = 0x20000000, LENGTH = 12k
}
/*The stack points where sram ends. It goes from top to bottom.*/
_estack = ORIGIN(sram) + LENGTH(sram);

/*
Use of Align 
- Cortex-M0 processor will throw hardfault when you try to do an unaligned
memory access(access to a memory address which is not multiple of 4 byte)
- Unaligned accesses to code memory definitely cause a HardFault but an unaligned 
access to data can be handled but with a performance penalty
- gcc mostly adds the required padding after variables or structures generated
struct packed_example {
        uint8_t  a;
        uint32_t b;
        uint8_t  c;
    } example;
Assume that you initialise all fields of example, a will have 3 bytes of empty padding
after it stores the first value. So if you want optimisation of memory this is something
you could do and change around the order of the data structure(so here have b,a,c).
- In the linker script you typically just need to start at a multiple of 4 and normally
 the alignment will be fine
- One thing that could destroy this is adding packed data structures where you tell
 gcc to add no padding so this would lead to performance penalty on access
- code being aligned isn't always guaranteed it seems to be architecture dependent 
rather than spending time on reading about this just put align
- Always put align at start and end of section -> a lot of the aligns will be 
unnecessary but just add them for safety(you may add new sections in the middle and forget the aligns ther)
*/
SECTIONS {
    .vectors : {
        . = ALIGN(4);
        KEEP(* (.vectors))
        . = ALIGN(4);
    } > flash
    /*
    There will be garbage collection performed by linker because --gc-sections flag 
    so it will see that stuff in vectors section(vector table) is never used in code 
    so it will remove this stuff.
    
    Both compiler and linker may remove functions which they consider to be unused 
    */
    .text : {
        . = ALIGN(4);
        * (.text*)
        . = ALIGN(4);
    } > flash
    .rodata : {
        . = ALIGN(4);
        * (.rodata*)
        . = ALIGN(4);
    } > flash /* rodata is not stored in RAM for space */
    /* 
    Each of the LHS is creating a section in the output file called that secname
    and taking from all the files(what the * does) the code in specified section inside
    to the current section. So it adds all the code in .vectors section in all the files to .vectors of output fie 
    Also the format in section matches how declarations come up
    So .vectors first, then .text, ... in the actual memory
    */
    .data : {
        . = ALIGN(4);
        /* data section holds initialised global variables */
        _startDataRAM = .;/* Semicolon must follow an assignment */
        * (.firstData)
        *(.data SORT(.data.*)) /*
        - Sometimes you can specify to compile code 
        so that each variable is in its own section which makes it easier to remove
        if a variable is unused meaning you can save space 
        - SORT means all these sections 
        get's added in a consistent order no matter which order files processed
        - .data is added first and then these .data.* sections added
        */
        . = ALIGN(4);
        _endDataRAM = .;
    } > sram AT > flash
    _startDataFLASH = LOADADDR(.data);
    /*
    - The . is the output location counter so it stores the thing at the end of
    the data written. The > sram controls what the . is set to so here the . is set
    to the start of sram because of > sram. 
    - The assignment _startDataRAM = ... creates a variable which you can use later
    - The AT > flash tells you where to load the data in, if there is no AT the
    load address = address based from the >(so for the >flash, it records what's 
    added to the flash to generate the virtual address at the end of what's added 
    and then as there is no AT it just assumes that the virtual address is load address)
    - _startDataRAM, ... and the other _ can be accessed as variables -> they will
    be used in the copy code of the flash to the RAM
    - Currently nothing in code uses .firstData as a section so it just gets ignored
    */
    .bss : {/* unitialised global variables will be given 0 by default*/
        . = ALIGN(4);
        _startBSS = .;
        *(.bss SORT(.bss.*)) /* this and next line have same meaning as above with the data*/
        . = ALIGN(4);
        _endBSS = .;
    } > sram 
    /*
    - This is never loaded into flash(no AT > flash)! We already know what initial 
    values these variables will take(0) so there is no need to load it into flash.
    - We just do this to figure out where it starts and ends so we can write 0s
    */

}