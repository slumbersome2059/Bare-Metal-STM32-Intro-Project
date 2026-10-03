#include "hal.h"

#define RCC_BASE 0x40021000
#define ADC1_BASE 0x40012400
/* System clock frequency used by the FreeRTOS port. */
uint32_t SystemCoreClock = 48000000UL;

//These describe different registers in the MCU which allow you to enable different peripherals
//They allow clock management and allow you to reset parts of the circuit
//By configuring registers in the MCU you can enable GPIO banks
//To save power in STM32 all peripherals are turned off but not the case in most other MCUs
volatile uint32_t* RCC_IOPENR = (volatile uint32_t*)(RCC_BASE + 0x34) ;//enables clock to GPIO banks
//They allow clock management and allow you to reset parts of the circuit
//This gives a clock source to components    
volatile uint32_t* RCC_APBENR1 = (volatile uint32_t*)(RCC_BASE + 0x3C) ;
volatile uint32_t* RCC_APBENR2 = (volatile uint32_t*)(RCC_BASE + 0x40) ;


volatile uint32_t* ADC1_ISR = ((volatile uint32_t*)(ADC1_BASE + 0x00));
volatile uint32_t* ADC1_CR = ((volatile uint32_t*)(ADC1_BASE + 0x08));
volatile uint32_t* ADC1_CFGR1 = ((volatile uint32_t*)(ADC1_BASE + 0x0C));
volatile uint32_t* ADC1_SMPR = ((volatile uint32_t*)(ADC1_BASE + 0x14));
volatile uint32_t* ADC1_CHSELR = ((volatile uint32_t*)(ADC1_BASE + 0x28));
volatile uint32_t* ADC1_DR = ((volatile uint32_t*)(ADC1_BASE + 0x40));
volatile uint32_t* ADC1_CCR = ((volatile uint32_t*)(ADC1_BASE + 0x308));
volatile uint32_t* ADC1_CALFACT = ((volatile uint32_t*)(ADC1_BASE + 0xB4));

volatile uint32_t* NVIC_ISER = ((volatile uint32_t*)(0xE000E100));

SYS_TICK* const SYS_TICKp = (SYS_TICK* const)0xE000E010;

int powInt(int base, int exp){
    int ans = 1;
    while(exp){
        ans *= base;
        exp--;
    }
    return ans;
}

/*
I think this should not have been made into a function, code like 5*2 will now have to be executed
and pow will be executed so it would have been more efficient to just call the normal code.
*/
void setReg(volatile uint32_t* reg, int index, int stride, int val){
    /*
    Sets bits on a register to 1 or 0.
    */
    int oneMask = (int)((powInt(2, stride)) - 1);
    *reg &= (uint32_t)(~(((oneMask)) << (index*stride)));//this 0s out what is in bits you will change so that following mask will work and it does not affect other bits
    *reg |= (uint32_t)(((val)&(oneMask)) << (index*stride));
}

GPIO *getGPIO(char bank){
    //GPIO *gpioPins[GPIO_BANK_NUMBER] = {0, 0, 0, 0, 0};
    int base = 0x50000000;
    int offset = 0x400;
    int i = bank - 'A';
    return (GPIO *)(base + i*(offset));
} 
void setModeGPIO(char bank, int pinNum, GPIO_mode gM){
    GPIO *gpioBank = getGPIO(bank);
    //pin numbers start from 0 so you can have A0 and mode for pin number stored as two bits in pinNum*2 and pinNum* + 1
    setReg(&(gpioBank->MODER), pinNum, 2, gM);
    //U is added after 3 to make it unsigned to avoid any strange errors that may happen
}

void setAltFuncGPIO(char bank, int pinNum, int afNum){
    GPIO *gpioBank = getGPIO(bank);
    if(pinNum <= 7){
        setReg(&(gpioBank->AFRL), pinNum, 4, afNum);
    }else if(pinNum >= 8 && pinNum <= 15){
        setReg(&(gpioBank->AFRH), pinNum - 8, 4, afNum);//setReg requires an index so you need to - 8
    }
    
}

void writeGPIO(char bank, int pinNum, int val){
    GPIO *gpioBank = getGPIO(bank);
    gpioBank->BSRR = (1U << (pinNum+(val ? 0: 16)));
}
/*

    - Making a BSRR pin 1 will mean a write or reset happens to the pin number
    on ODR which stores the output and a 0 means ODR will not be assigned
    - if you pass in 0, to write 0 you need to reset so you need to make 1 the bits of 16 to 31
    on BSRR which will reset the bit on ODR
    - There are only 0 to 15 bits on ODR
    - You can modify the ODR directly but writing one bit require a read(taking ODR value 
    into register as you want to preserve the values in other bits), modify(bitwise 
    OR), write(back to ODR) series of steps
    - This violates atomicity so after you have read if there is an interrupt and 
    it modifies some other bit of same ODR and then your modify and write happens
    , this would mean that the modification made by interrupt would be lost as you
    modify old read and then write that back
    - With BSRR you directly write(look at the '=') to the register and there is 
    no read or modify loop when you change data of BSRR in the software and in same 
    cycle of writing to BSRR the ODR value is updated as well
    - The whole bit (re)/setting of ODR is done in one cycle so atomic
    */
//Initialises the counting process and returns a success value 
//on whether it worked or not (-1 for failure and 0 for success)
int systickInit(int ticks){
    /*
    - Set the reload value in SYST_RVR
    - Clear SYST_CVR(this means write it to 0 -> writitng ANY VALUE to SYST_CVR will
     clear it to 0)
    - Enable the counting process through SYST_CSR
    - For every cycle, it will (detect if CVR is 0 and so reset to value in SYS_RVR 
    then do following) decrement the value in SYST_CVR and there is a trigger that activates 
    when counter decrements from 1 to 0 which sets the SYSTICK exception pending status to true
     which should run the exception handler when possible
    */
    if(ticks - 1 > 0xffffff || ticks <= 1){
        return -1;//the systick register for the current value has 32 bits but only 24 of them are used for storage
    }
    SYS_TICKp->SYST_RVR = (uint32_t)(ticks-1);//One cycle is used to run the interrupt handler so you subtract off one for the cycles that main runs
    SYS_TICKp->SYST_CVR = 0;//clrea
    SYS_TICKp->SYST_CSR = 0x7;
    /*
    - this is 111 becuase when 0 bit is 1 you enable counting, 
    - 1st bit is 1 means interrupt generation and you call handler 
    - 2nd bit is 1 means you use processor clock
    */
   return 0;
    
}

bool timerExpire(uint32_t* lastTick, uint32_t prd, uint32_t timeNow){
    bool entered = false;
    while(timeNow - *lastTick >= prd){
        entered = true;
        *lastTick += prd;
    }
    return entered;
}
/*
- Don't create an expire function like below because it does not work with overflows
- So for example when expirationTime is very high and timeNow reaches it 
,expirationTime goes very low and we keep returning true because second
 if check won't succeed
- There's also the problem that when timeNow overlaps so much that we just keep returning false
- The way to deal with is to avoid the comparison between timeNow and expirationTime 
bool timerExpire(uint32_t* expirationTime, uint32_t prd, uint32_t timeNow){
    if(*expirationTime == 0){//for first time that timerExpire called
        *expirationTime = timeNow + prd;
    }
    if(timeNow < *expirationTime){
        return false;
    }
    *expirationTime = prd - (timeNow - *expirationTime)%(prd) +timeNow; 
    return true;
}
*/

void adc_setup(char bank, int pinNo){
    //Prescaling the ADC clock to achieve necessary duty cycle
    *ADC1_CCR |= (1 << 18);
    //Enabling ADC clock
    *RCC_APBENR2 |= (1 << 20);

    //Setting GPIO correctly
    setModeGPIO(bank, pinNo, GPIO_MODE_ANALOG);

    
    
    //setReg(&(ADC1CR), 0, 1, 0);//Disabling ADC,
    
    setReg(ADC1_CFGR1, 15, 1, 0);//Setting AUTOOFF to 0, AUTOOFF means power on and off automatic

    //setReg(&(ADC1_CFGR1), 0, 1, 0);//Disabling using DMA to manage converted data 
    //When doing multiple conversions you use this to create a DMA request after each conversion so that data can be stored in a specified place
    setReg((ADC1_CR), 28, 1, 1);//Enable ADC Voltage Regulator (ADVREGEN = bit 28)
    /*
    This bit enables the voltage regulator on to controller.
    You need to wait 
    */
    for (volatile int i = 0; i < 1000; i++); // Wait for regulator startup delay (~20 µs)
    setReg(ADC1_CR, 31, 1, 1);//calculates calibration factor which is applied to ADC(removes offset error which varies from chip to chip)

    while ((*ADC1_CR) & ((uint32_t)(1 << 31))){};//waits for calibration to finish
    
    *ADC1_CALFACT = ((*ADC1_CALFACT) + ((uint32_t)1));
    
    // This is the ADCAL flag(the 31), cleared when calibration finished, when it is set it starts ADC calibration
    
    
    *ADC1_ISR = 1;//Writing 1 to ADRDY bit  clears ADRDY flag
    setReg(ADC1_CR, 0, 1, 1);// Set ADEN = 1, this enables the ADC
    while (!(*ADC1_ISR & (1))){}; 
    /*
    This bit is set by hardware after the ADC has been enabled (ADEN = 1) and 
    when the ADC reaches a state where it is ready to accept conversion requests
    */

    //print_reg_vals(sizeof(uint16_t)); -> DOESN'T WORK
    /*
    - There are channels from GPIO pins and internal inputs(internal ref voltage) 
    and these can be read by the ADC(if configured through CHSELR) 
    */
    *ADC1_ISR = (1 << 13);//CCRDY cleared when you write 1
    *ADC1_CHSELR |= (1U << pinNo);
    while (!(*ADC1_ISR & (1 << 13))){};//Wait till CCCRDY flag is set which tells you when channel config has been done
}


uint16_t analog_read(){
    /*
    - the setup and read code is only useful for reading from one pin and you 
    should be careful of changing channels(read datasheet, changing channels during conversions are a problem)
    - both the code needs to be changed for reading from multiple
    - you need to enable clocks(ADC and GPIOA) and do adc_setup before this
    */
    setReg(ADC1_CR, 2, 1, 1);//Enabling ADSTART to start conversion, bit is cleared when EOS flag is set
    // In the setup it could be that you have many conversions because you are reading from many channels
    // You can specify order in which these channels are scanned(this may help if you have sensors which are dependent on other sensors )
    // So to distinguish we have an EOS flag and a EOC flag
    //here we are only reading from one channel -> that's what is set up

    while (!(*ADC1_ISR & (1 << 2))){};//loop till EOC is done
    uint16_t adc_value = (uint16_t)(*ADC1_DR);
    while (!(*ADC1_ISR & (1 << 3))){};//loop till EOS is done
    *ADC1_ISR = 1 << 3;//EOS is set to 0 when you write 1 to it, EOC automatically cleared when we read to it
    
    //Because we use single mode conversions only start after ADSTART is set
    return adc_value;
}

//Only uart1 and uart2 have RX and TX ports mapped to GPIO pins on stm32c031c6 
UART* const uart2 = (UART*)0x40004400;

void delay(int N){
    while(N--){
        asm("nop");
    }
}

void initSerialMonitor(){
    //Setting up GPIO pins
    setModeGPIO('A', 2, GPIO_MODE_AF);
    setModeGPIO('A', 3, GPIO_MODE_AF);
    setAltFuncGPIO('A', 2, 1);
    setAltFuncGPIO('A', 3, 1);
    //Enabling USART peripheral
    *RCC_APBENR1 |= (1 << 17); //you are only changing one bit so no need to zero things out(would be necessary for storing eg 01)
    (void)*RCC_APBENR1; // Dummy read forces CPU to wait for clock stabilization
    uart2->CR1 = 0;//UART(from setting UE bit to 0) needs to be disabled for some bits to be set
    uart2->BRR = (uint32_t)(SystemCoreClock/115200);
    uart2->CR1 = 13;
}

void writeToSerialMonitor(char* msg){
    while(*msg != 0){
        uart2->TDR = (uint8_t)(*msg);
        msg++;
        while((uart2->ISR & (1 << 7)) == 0){
            /*
            This is TXE bit which is used to show TDR is free and the data in there has 
            been moved to shift register so you can write there
            There is a TC bit which shows the whole transmission is complete so shift register is empty 
            and TX line is IDLE. This is used right at the end so that you don't disable 
            the USART when there is data in shift register for example. But TC does not seem to work
            on STM32 on Wokwi when I use it here. For example, using TC should mean loop runs longer but no 
            change in output while only first letter gets printed in reality.
            */
            delay(1);
        };
    }    
}

//extern UBaseType_t uxQueueMessagesWaitingFromISR(const QueueHandle_t xQueue);
//extern BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue, void *const pvBuffer, BaseType_t *const pxHigherPriorityTaskWoken);
extern QueueHandle_t systQueue;
void writeWordISR(){//This is an interrupt service routine
    //When using NVIC peripheral interrupts you will usually need to clear the pending interrupt
    //Whenever the interrupt signal is asserted the interrupt is pending, and you can control it's state with some registers
    //Here we will automatically clear the interrupt by writing to TXE so doing it twice could result in problems
    //writeToSerialMonitor("DS\n");
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if(uxQueueMessagesWaitingFromISR(systQueue)){
        char s = '0';
        xQueueReceiveFromISR(systQueue, &s,&xHigherPriorityTaskWoken);        
        uart2->TDR = (uint8_t)(s);
    }else{
        uart2->CR1 &= ~(1U << 7); // Disable TXEIE when queue is empty
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);//context switch if the thing you did caused some higher priority task to unblock
    //In an ISR the switching of tasks may not immediately happen in between ticks(it woudl in tasks)
    //This is used to make sure you don't have to wait till end of tick to switch task

}

void enableInterrupt(int line){//look at programming manual docs for this, PM0223
    *NVIC_ISER = (1 << line); //this is just a reg to enable an interrupt, other register for clearing, 0 has no effect
    //you seem to need to do this only for the peripheral interrupts and not the core interrupts(you don't seem to have to do any kind of management for these)
}

void setTXEInterruptsUSART(){
    uart2->CR1 |= (1 << 7);//Enables TXEIE which will give you an interrupt whenever TXE is ON
    //Initially even when TXE is 0, it seems like there is an interrupt when you enable TXEIE
    //The code after the enabling only executes after the first byte has been written to UART 
}

void enable_gpio_clock(){
    *RCC_IOPENR |= 1;//GPIOA, enabling done here is more concise because in function you don't exactly know which bank to enable
}