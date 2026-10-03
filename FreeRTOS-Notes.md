# FreeRTOS
## Scheduling Algo
- Scheduling algorithm is pick the highest priority available to execute at that time and keep executing it till it blocks
- It will replace some task currently executing if a higher priority one comes along(when hp task becomes free in a task this will happen, when hp task free in interrupt you need to use `pxHigherPriorityTaskWoken` and more info given on 7.2.4 of https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch07.md )
- For tasks of equal priority there will be round robin and they will keep switching arround
## Tasks
- the freeRTOS equivalent of processes
- When task gets unblocked it starts from same place it was blocked
## Queues
- Used to transfer data between tasks or interrupts and tasks
- Different APIs for tasks and interrupts(functiosns to do with queues have less overhead when used with tasks)
- When task attempts to write to a queue and its full, it gets blocked, when task attempts to read and its empty it gets blocked -> when the data is available the task gets unblocked if its blocked for more than the block time(block time specified in the function doing reading or writing, this can be set to infinity with portMAX_DELAY)
- When it unblocks it starts of at same place of blocking
## Semaphores
- It's like a queue but you don't care about the data in the queue just the length of items in the queue
- The same idea of tasks waiting when semaphore is 0 or max value is true
- When max value is greater than 1 you have counting semaphore and for 1 it is a binary semaphore
## Direct to Task Notifications
- Each RTOS task has array of notifications -> each notification has a state(boolean, pending or not) and a value(32-bit)
- Sending a direct to task notification to a task sets the state of the target task notification to 'pending' 
- A task can block on a task notification to wait for that notification's state to become pending
- Compared to semaphore and queues: more efficient in speed and memory, only one task is recipient, sender can't block if send doesn't work 
- Notification is cleared immediately once task becomes unblocked because its waiting for a notification
## Stream and Message Buffers
- Like queues but used for sig
## Questions
- Stream buffers are an [RTOS task](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/00-Tasks-and-co-routines/) to RTOS task, and interrupt to task communication primitives
- optimised for single reader single writer scenarios(even in single core its important to place things in a critical section and not have any block time)
- Stream buffers pass a continuous stream of bytes. Message buffers pass variable sized but discrete messages. Message buffers use stream buffers for data transfer.
- Stream buffers would have been more efficient instead of queues for my uart task
