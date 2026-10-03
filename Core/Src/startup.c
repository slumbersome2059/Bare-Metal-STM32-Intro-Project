extern long _startDataRAM, _endDataRAM, _startDataFLASH, _startBSS, _endBSS;

/*
This code will take stuff in .data and .bss in flash memory and load it into RAM.
This is stuff that needs to be stored in non-volatile memory so that its durable so it will be in flash
but to be used by MCU when chip is booted it needs to be in RAM.
*/
extern int main(void);
__attribute__((noreturn)) void _reset(void){
    /*
    naked 
    - means no function epilogue(restores stack for parent) and no function prologue(this is where stack is set up eg required number of bytes for variables allocated)    
    - you use this when you want to write function in assembly code(you can't use any local variables or if statements inside function because of naked)
    - you would typically write the prologue and epilogue in assembly
    - The code in this function should be in assembly in a __asm__{code} statement
    - Sometimes C code will work but its unreliable and should never be trusted(we get lucky here)
    - Prologue also saves the current context like registers but as this function is 
    run at the start the registers will have dummy data so storing them in RAM is a waste of time
    - Epilogue also not necessary(an epilogue is generated even though no return) so naked
    can save space
    
    
    no_return
    - this code will never go back to caller(except when there are exceptions in code)
    difference to void is that function declared with void can go back to caller
    - mainly used for optimisations like cutting out code that is written after the function call(that code will never execute)
    */
    long* src = &(_startDataFLASH);
    /*
    - Linker script variables add the variableName to the symbol table and the
    value that the variable is assigned to, is stored in an entry with the variableName 
    in the symbol table(so the value that the variableName is assigned to is its address not value), 
    typically for declarations in C in symbol table you would store varName = memory address of value stored
    but with a linker script variable there is no memory assigned to the variable
    - This is why in source code you access &_startDataRAM and same for other linker 
    script variables because all you want is the address of the variable because the value of 
    linker script variable is stored where you would typically store an address of a typical variable
    - using the value of a variable in source code(so if you have
    int a = 5; int b = a + 2) will get address of a from symbol table and use the 
    address to get the value of a 
    - So plainly using _startDataRam will cause problems because there is no memory
    assigned to _startDataRam so you can't access a value like you typically would for 
    high level variables
    */
    for(long* dst = ((&_startDataRAM)); dst < &_endDataRAM; dst++){//addresses of _startDatRAM and endDataRAM are not what you want to copy into they are like goalposts
        *dst = *src;
        src += 1;
    }
    for(long* dst = ((&_startBSS)); dst < &_endBSS; dst++){//addresses of _startDatRAM and endDataRAM are not what you want to copy into they are like goalposts
        *dst = 0;
    }
    //reset does this moving because RAM is faster than flash to read from and write to
    main();
    for(;;);//infinite loop

};

extern void _estack(void);
extern void systickHandler(void);
extern void vPortSVCHandler( void );
extern void xPortPendSVHandler( void );
extern void xPortSysTickHandler( void );
extern void writeWordISR( void );
__attribute__((used, section(".vectors"))) void (* const tab[16+32])(void) = 
    {_estack, _reset, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    vPortSVCHandler, 0, 0, xPortPendSVHandler, xPortSysTickHandler, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, writeWordISR, 0, 0, 0};
//only first two handlers defined rest are zeroed
//Every handler is a handler for a hardware interrupt, apart from the first two -> second one is the code which starts bootup and first is the initial stack pointer
//So this table is also read at the start for bootup
//There are 16 ARM handlers and 31 peripheral handlers specific to stm32c0-series boards
//The section part means vector table is put in .vectors rather than .text after compilation(we can make linker put .vectors code at start of firmware and so start of flash where it needs to be)
//What is represented by a GPIO_PIN(the 2 bits in all the registers corresponding to one number)?
/*
- Also in the symbol table the function name will be stored 
with the address of the function so you can just pass the function name in and you dont have to do &_estack
- This is the same thing with array names as well so if you had 
arrayName[] and then do arrayName = .; in linker script you would just access 
arrayName for the address
- used attribute is to make the compiler write code for function even if it is 
not referenced(tab is used in bootup and invisible to compiler)
- I haven't seen volatile to defend against what the used attribute does
*/
