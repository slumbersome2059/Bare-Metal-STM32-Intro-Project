//#define CLOCK_FREQ 12000000UL
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "FreeRTOS.h"
#include "task.h"

/* System clock frequency used by the FreeRTOS port. */
uint32_t SystemCoreClock = 48000000UL;

#define USARTDIV 48000000/115200
const int GPIO_BANK_NUMBER = 5;
const int CLOCK_FREQ = 48000000;


//Only uart1 and uart2 have RX and TX ports mapped to GPIO pins on stm32c031c6 
UART* const uart1 = (UART*)0x40013800;
UART* const uart2 = (UART*)0x40004400;
UART* const uart3 = (UART*)0x40004800;
UART* const uart4 = (UART*)0x40004C00;


static volatile uint32_t s_ticks = 0; 
void systickHandler(){
    s_ticks++;
}



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
    uart2->BRR = (uint32_t)(48000000/115200);
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
             on STM32 when I use it here. For example, using TC should mean loop runs longer but no 
             change in output while only first letter gets printed in reality.
            */
            delay(1);
        };
    }    
}

void print_reg_vals(uint32_t reg_vals){
    writeToSerialMonitor("\n");
    char msg[33];
    msg[32] = '\0';
    for(int i=31; i>=0; i--){
        msg[31-i] = '0'+(char)(((reg_vals>>i) & (1)));
    }
    writeToSerialMonitor(msg);
}

/* 
Hook prototypes 
- Not used now so not implemented
*/
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

void configureTimerForRunTimeStats(void)
{

}

unsigned long getRunTimeCounterValue(void)
{
    return 0;
}

void uint_to_str(uint16_t val, char *str) {
    /*
    This loops through each of the powers of 10 and repeatedly does subtraction if val is less than the power.
    It then displays the string from this.
    The length of the array passed in should be 1 more than the space that the number 
    takes because you need space for \0. 
    */

    // Powers of 10 for up to a 16-bit unsigned integer
    static const uint16_t powers[5] = {
        10000U, 1000U, 100U, 10U, 1U
    };
    
    int idx = 0;
    bool leading_zero = true;

    if (val == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    for (int i = 0; i < 5; i++) {
        uint16_t p = powers[i];
        char count = '0';
        
        while (val >= p) {
            val -= p;
            count++;
        }
        
        // Skip leading zeros
        if (count != '0' || !leading_zero) {
            leading_zero = false;
            str[idx++] = count;
        }
    }
    
    str[idx] = '\0';
}
void systemInit(void){
    systickInit(CLOCK_FREQ/1000);
    *RCC_IOPENR |= 1;//GPIOA, enabling done here is more concise because in function you don't exactly know which bank to enable
    setModeGPIO('A', 10, GPIO_MODE_OUTPUT);
    initSerialMonitor();
    writeToSerialMonitor("HELLO\n");
    adc_setup('A', 0);
}

void vBlinkTask(void *pvParameters) {
    //you have a function for each task which is implemented as an infinite for loop
    configASSERT(pvParameters == NULL);
    bool on = true;
    for (;;) {
        writeGPIO('A', 10, on);
        on = !on;
        // Task is blocked(waiting) for this amount of time then replaced from start I think
        // this task executes time taken before this line + 500 again
        // If you want the WHOLE task to execute for just 500 ms then use vTaskDelayUntil
        vTaskDelay(pdMS_TO_TICKS(500));

    }
}

void vUartTask(void *pvParameters) {
    configASSERT(pvParameters == NULL);
    for (;;) {
        writeToSerialMonitor("INSIDE TASK");
        vTaskDelay(pdMS_TO_TICKS(500));
        writeToSerialMonitor("Called again");//Test to see if called from start or here
    }
}

int main(void){
    systemInit();
    xTaskCreate(vUartTask, "Uart", 50, NULL, 1, NULL);
    xTaskCreate(vBlinkTask, "Blink", 50, NULL, 2, NULL);
    vTaskStartScheduler();
    /*
    - the last argument is a pointer to the task datatype and you can use it in 
    later task methods(pointer not needed here so null is passed in)
    - stack depth is 50 here, this is depth each row(out of 50 rows) is 4 bytes here(this is port specific and dependent on architecture)
    
    */
    

    /*
    systickInit(CLOCK_FREQ/1000);
    *RCC_IOPENR |= 1;//GPIOA, enabling done here is more concise because in function you don't exactly know which bank to enable
    setModeGPIO('A', 10, GPIO_MODE_OUTPUT);
    initSerialMonitor();
    writeToSerialMonitor("HELLO\n");
    adc_setup('A', 0);
    writeToSerialMonitor("HELLO\n");
    
    static bool on = true;
    uint32_t prd = 2000;
    uint32_t lastTick = s_ticks;
    uint32_t count = 0;

    while(1){
        if(timerExpire(&lastTick, prd, s_ticks)){
            char snum[16];
            uint_to_str(analog_read(), snum);
            writeToSerialMonitor(snum);
            writeToSerialMonitor("\n");
            writeGPIO('A', 10, on);
            on = !on;
            count++;
        }
        
        - you can do other stuff here -> benefit of doing this compared to delay 
        from empty loop
        - but delay loop better for accuracy because if the code below takes a 
        considerable time the next LED blink will have to pay for it
        
    }
    return 0;
    */
    
}

