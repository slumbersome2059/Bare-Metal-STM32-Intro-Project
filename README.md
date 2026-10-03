# README
## Project Description
This repository is for a project where data is read from analog temperature sensor, and the results are written to a UART monitor and there is an LED blink when data is ready for processing. To ensure blinking for a reliable period I learnt to use the SYSTICK peripheral. I did not use any normal libraries and wrote all the code for the startup and hardware abstraction layer. Moreover, I learn about freeRTOS functionalities like queues, semaphores, task notifications and scheduling. I also learnt how to use linker script to load the firmware into memory. Furthermore, I enhanced my knowledge on Makefile fundamentals in this project. The tutorial https://github.com/cpq/bare-metal-programming-guide/tree/main was very helpful for this project. My board was different to the one on the guide so I had to use the datasheet to figure out how to use the new registers on my board.
## Skills Learnt
- Makefile
- Linker Script
- UART
- SYSTICK
- GPIO
- FreeRTOS
- Using interrupts and interrupt handlers
## Running the project
Load the binary file into the Wokwi simulator given here: https://wokwi.com/projects/new/st-nucleo-c031c6 . You can also watch stuff from the project in the videos section.
