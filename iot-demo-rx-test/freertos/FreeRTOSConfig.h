/*
 * FreeRTOSConfig.h - GR-SAKURA RX63N (HOCO 50MHz)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* CPU */
#define configCPU_CLOCK_HZ              ( 50000000UL )
#define configPERIPHERAL_CLOCK_HZ       ( 50000000UL )
#define configTICK_RATE_HZ              ( ( TickType_t ) 1000 )

/* Tick: CMT0 (ベクタ28 = 0x70) */
#define configTICK_VECTOR               _CMT0_CMI0

/* Scheduler */
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configMAX_PRIORITIES            5
#define configMINIMAL_STACK_SIZE        ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE           ( ( size_t ) ( 32768 ) )
#define configMAX_TASK_NAME_LEN         12

/* Tick type */
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS

/* Features */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_QUEUE_SETS            0
#define configUSE_RECURSIVE_MUTEXES     0
#define configQUEUE_REGISTRY_SIZE       0
#define configUSE_TASK_NOTIFICATIONS    1

/* Memory */
#define configSUPPORT_STATIC_ALLOCATION     0
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configAPPLICATION_ALLOCATED_HEAP    0

/* Hook functions */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configUSE_MALLOC_FAILED_HOOK    0
#define configCHECK_FOR_STACK_OVERFLOW  2

/* Timers */
#define configUSE_TIMERS                0

/* Co-routines */
#define configUSE_CO_ROUTINES           0

/* Interrupt nesting */
#define configKERNEL_INTERRUPT_PRIORITY         1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     4

/* Optional functions */
#define INCLUDE_vTaskPrioritySet        0
#define INCLUDE_uxTaskPriorityGet       0
#define INCLUDE_vTaskDelete             0
#define INCLUDE_vTaskSuspend            1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1
#define INCLUDE_xTaskGetSchedulerState  0
#define INCLUDE_uxTaskGetStackHighWaterMark  1

/* Assert */
#define configASSERT( x )  if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

#endif /* FREERTOS_CONFIG_H */
