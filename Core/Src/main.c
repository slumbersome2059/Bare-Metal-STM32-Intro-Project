//#define CLOCK_FREQ 12000000UL
#include <stdint.h>
#include <stdbool.h>
#include "hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define MAX_NUMBER_LENGTH 6
#define QUEUE_NUMBER_LENGTH 10
#define UART_PERIP_INTERRUPT_LINE 28
const int GPIO_BANK_NUMBER = 5;

QueueHandle_t systQueue;
TaskHandle_t uartTask;
TaskHandle_t blinkTask;


static volatile uint32_t s_ticks = 0; 
void systickHandler(){
    
    s_ticks++;
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
    systickInit(((int)SystemCoreClock)/1000);

    enable_gpio_clock();
    setModeGPIO('A', 10, GPIO_MODE_OUTPUT);
    initSerialMonitor();
    adc_setup('A', 0);
    enableInterrupt(UART_PERIP_INTERRUPT_LINE);
    systQueue = xQueueCreate(QUEUE_NUMBER_LENGTH*MAX_NUMBER_LENGTH, sizeof(char));
}

enum map_task_notif_blink {ITEM_ADDED};
void vBlinkTask(void *pvParameters) {
    //you have a function for each task which is implemented as an infinite for loop
    configASSERT(pvParameters == NULL);
    for (;;) {
        ulTaskNotifyTakeIndexed(ITEM_ADDED, pdTRUE, portMAX_DELAY);
        //writeToSerialMonitor("B\n");
        writeGPIO('A', 10, true);
        vTaskDelay(pdMS_TO_TICKS(10));
        writeGPIO('A', 10, false);
        // Task is blocked(waiting) for this amount of time then replaced from same place it left off
        // this task executes time taken to get here before this line + 500 again
        // If you want the WHOLE task to execute for just 500 ms then use vTaskDelayUntil
        //writeToSerialMonitor("Done");
        

    }
}

enum map_task_notif_uart {ADDED_FROM_EMPTY};
void vUartTask(void *pvParameters) {
    configASSERT(pvParameters == NULL);
    for (;;) {
        //I couldn't find a way to make this task block on when the queue is empty so I had to use notifications or peek
        //I could have used notification but I think in terms of memory it would have been more inefficient
        ulTaskNotifyTakeIndexed(ADDED_FROM_EMPTY, pdTRUE, portMAX_DELAY);
        //writeToSerialMonitor("U\n");
        setTXEInterruptsUSART();
    }
}

void vTempSensorTask(void *pvParameters) {
    configASSERT(pvParameters == NULL);
    
    for (;;) {
        uint16_t v = analog_read();
        char strV[MAX_NUMBER_LENGTH];
        uint_to_str(v, strV);
        int i = 0;
        UBaseType_t messagesStart = uxQueueMessagesWaiting(systQueue);
        while(strV[i]){
            xQueueSendToBack(systQueue, &(strV[i]), portMAX_DELAY);
            i++;
        }
        char newLine = '\n';
        xQueueSendToBack(systQueue, &newLine, portMAX_DELAY);
        if(messagesStart == 0){//if queue is not empty eventually the uart task will process it away, this task may interrupt uart because it has higher priority
            xTaskNotifyGiveIndexed(uartTask, ADDED_FROM_EMPTY);    
        }
        xTaskNotifyGiveIndexed(blinkTask, ITEM_ADDED);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void){
    systemInit();
    xTaskCreate(vTempSensorTask, "TempSensor", 50, NULL, 3, NULL);
    /*
    - the last argument is a pointer to the task datatype and you can use it in 
    later task methods(pointer not needed here so null is passed in)
    - stack depth is 50 here, this is depth each row(out of 50 rows) is 4 bytes here(this is port specific and dependent on architecture)
    - xTaskCreate returns pdPass or pdFail
    */
    xTaskCreate(vUartTask, "Uart", 50, NULL, 1,  &uartTask);
    xTaskCreate(vBlinkTask, "Blink", 50, NULL, 2, &blinkTask);
    vTaskStartScheduler();
    

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

