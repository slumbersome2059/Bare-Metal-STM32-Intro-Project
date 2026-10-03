//This is the hardware abstraction layer, it gives structures which can be used to access the memory to which registers are mapped to
//It also usually includes functions to interact with hardware
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


//GPIO code
struct GPIO{
    volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFRL, AFRH;
};

struct SYS_TICK{
    volatile uint32_t SYST_CSR, SYST_RVR, SYST_CVR, SYST_CALIB;  
};

struct UART{
    volatile uint32_t CR1, CR2, CR3, BRR, GTPR, RTQR, RQR, ISR, ICR, RDR, TDR, PRESC;
    /*
    - BRR -> set the baud rate, for UART and USART this is the amount of data transmitted per second
    - GTPR -> sets the guard time value(transmission complete flag set after this 
    time is drained), and sets prescaler value which divides system clock -> both these values are only
     accessed in certain modes and not in normal mode
    - RTOR -> receiver timeout register, sets a flag after some time 
    where nothing is to be read
    - RQR -> used to make requests like discard data without reading it, or put USART in mute mode
    - ISR -> gives information on the status of the USART like a busy flag if there is comms on the RX line, or RX stack is full
    - ICR -> clears flags of ISR, same idea of BSRR used for ODR
    - RDR -> contains data character received
    - TDR -> contains data character to be transmitted
    - PRESC -> used to divide the input clock by some number 
    */
};




typedef struct ADC ADC;
typedef struct GPIO GPIO;
typedef struct SYS_TICK SYS_TICK;
typedef struct UART UART;
typedef enum {
    GPIO_MODE_INPUT,//you will be reading from these registers
    GPIO_MODE_OUTPUT, 
    GPIO_MODE_AF,//this maps the pin as input for or output of(whether I or O depends on AF number and the pin) some other peripheral like USART, SPI, ...
    GPIO_MODE_ANALOG//you just use the actual analog value of the GPIO pin rather than interpreting it a binary value
    //you could sample the value and take it as input for ADC
} GPIO_mode;




int powInt(int base, int exp);

/*
I think this should not have been made into a function, code like 5*2 will now have to be executed
and pow will be executed so it would have been more efficient to just call the normal code.
*/
void setReg(volatile uint32_t* reg, int index, int stride, int val);

GPIO *getGPIO(char bank);

void setModeGPIO(char bank, int pinNum, GPIO_mode gM);

void setAltFuncGPIO(char bank, int pinNum, int afNum);

void writeGPIO(char bank, int pinNum, int val);
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
int systickInit(int ticks);
bool timerExpire(uint32_t* lastTick, uint32_t prd, uint32_t timeNow);

void adc_setup(char bank, int pinNo);


uint16_t analog_read();



void delay(int N);

void initSerialMonitor();

void writeToSerialMonitor(char* msg);

//extern UBaseType_t uxQueueMessagesWaitingFromISR(const QueueHandle_t xQueue);
//extern BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue, void *const pvBuffer, BaseType_t *const pxHigherPriorityTaskWoken);
void writeWordISR();

void setTXEInterruptsUSART();

void enable_gpio_clock();

void enableInterrupt(int line);