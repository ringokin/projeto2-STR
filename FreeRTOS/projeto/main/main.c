/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

/* Visual studio intrinsics used so the __debugbreak() function is available
 * should an assert get hit. */
#include <intrin.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+Trace includes. */
#include "trcRecorder.h"

/*Include para utilizar os semáforos*/
#include "semphr.h"

/* This demo uses heap_5.c, and these constants define the sizes of the regions
 * that make up the total heap.  heap_5 is only used for test and example purposes
 * as this demo could easily create one large heap region instead of multiple
 * smaller heap regions - in which case heap_4.c would be the more appropriate
 * choice.  See http://www.freertos.org/a00111.html for an explanation. */
#define mainREGION_1_SIZE                     8201
#define mainREGION_2_SIZE                     23905
#define mainREGION_3_SIZE                     16807

/* This demo allows to save a trace file. */
#define mainTRACE_FILE_NAME                   "Trace.dump"

/*-----------------------------------------------------------*/

/*
 * This demo uses heap_5.c, so start by defining some heap regions.  It is not
 * necessary for this demo to use heap_5, as it could define one large heap
 * region.  Heap_5 is only used for test and example purposes.  See
 * https://www.FreeRTOS.org/a00111.html for an explanation.
 */
static void prvInitialiseHeap( void );

/*
 * Prototypes for the standard FreeRTOS application hook (callback) functions
 * implemented within this file.  See http://www.freertos.org/a00016.html .
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName );
void vApplicationTickHook( void );
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize );
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize );

/*
 * Writes trace data to a disk file when the trace recording is stopped.
 * This function will simply overwrite any trace files that already exist.
 */
static void prvSaveTraceFile( void );

/*-----------------------------------------------------------*/

/* When configSUPPORT_STATIC_ALLOCATION is set to 1 the application writer can
 * use a callback function to optionally provide the memory required by the idle
 * and timer tasks.  This is the stack that will be used by the timer task.  It is
 * declared here, as a global, so it can be checked by a test that is implemented
 * in a different file. */
StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];


/* Thread handle for the keyboard input Windows thread. */
static HANDLE xWindowsKeyboardInputThreadHandle = NULL;

/*-----------------------------------------------------------*/

/*
* Variáveis correspondentes ao semáforos que controlam o acesso às entradas e saídas das máquinas:
* var[x][y]:    var ->  entrada: diz respeito ao buffer de entrada da máquina
*                       saida: diz respeito ao buffer de saída da máquina
* 			    x ->    diz respeito à máquina (x + 1)
*               y ->    0: semáforo bloqueante do ponto de vista do robô
*                           (bloqueia quando o buffer de entrada está cheio ou o de saída está vazio)
* 					    1: semáforo bloqueante do ponto de vista da máquina
*                           (bloqueia quando o buffer de entrada está vazio ou o de saída está cheio)
* 
*               Exemplo: entrada[0][0] -> bloqueia o robô quando o buffer de entrada da máquina 1 está cheio
*                                           e é liberado quando a máquina consome o item da entrada
*                        entrada[0][1] -> bloqueia a máquina quando o buffer de entrada da máquina 1 está vazio
*                                           e é liberado quando o robô deposita um item na entrada
* 
* robo4SEM: semáforo indicativo de que há itens disponíveis para o robô 4 (na saída da máquina 2 ou da 3)
*/

SemaphoreHandle_t entrada[3][2], saida[3][2];
SemaphoreHandle_t robo4SEM;

void robo1(void* pvParameters) {
    (void)pvParameters; /* Just to remove compiler warnings. */

    while (1) {
        if (xSemaphoreTake(entrada[0][0], portMAX_DELAY) == pdTRUE) {
            vTaskDelay(100 / portTICK_PERIOD_MS); //pega o objeto
            printf("[Robo_1] \tpegou da entrada da celula\n");
            fflush(stdout);
            vTaskDelay(600 / portTICK_PERIOD_MS); //leva até a máquina e solta
            xSemaphoreGive(entrada[0][1]);
            printf("[Robo_1] \tdepositou na entrada da [Maquina_1]\n");
            fflush(stdout);
            vTaskDelay(500 / portTICK_PERIOD_MS); //volta para a posição inicial
        }
    }
}

void robo2(void* pvParameters) {
    (void)pvParameters; /* Just to remove compiler warnings. */

    while (1) {
        if (xSemaphoreTake(entrada[1][0], portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(saida[0][0], portMAX_DELAY) == pdTRUE) {
                vTaskDelay(100 / portTICK_PERIOD_MS); //pega o objeto
                printf("[Robo_2] \tpegou da saida da [Maquina_1]\n");
                fflush(stdout);
                xSemaphoreGive(saida[0][1]);
                vTaskDelay(600 / portTICK_PERIOD_MS); //leva até a máquina e solta
                printf("[Robo_2] \tdepositou na entrada da [Maquina_2]\n");
                fflush(stdout);
                xSemaphoreGive(entrada[1][1]);
                vTaskDelay(500 / portTICK_PERIOD_MS); //volta para a posição inicial
            }
        }
    }
}

void robo3(void* pvParameters) {
    (void)pvParameters; /* Just to remove compiler warnings. */

    while (1) {
        if (xSemaphoreTake(entrada[2][0], portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(saida[0][0], portMAX_DELAY) == pdTRUE) {
                vTaskDelay(100 / portTICK_PERIOD_MS); //pega o objeto
                printf("[Robo_3] \tpegou da saida da [Maquina_1]\n");
                fflush(stdout);
                xSemaphoreGive(saida[0][1]);
                vTaskDelay(900 / portTICK_PERIOD_MS); //leva até a máquina e solta
                printf("[Robo_3] \tdepositou na entrada da [Maquina_3]\n");
                fflush(stdout);
                xSemaphoreGive(entrada[2][1]);
                vTaskDelay(800 / portTICK_PERIOD_MS); //volta para a posição inicial
            }
        }
    }
}

void robo4(void* pvParameters) {
    (void)pvParameters; /* Just to remove compiler warnings. */

    int prox = 1, ant = 2;
    while (1) {
        if (xSemaphoreTake(robo4SEM, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(saida[prox][0], 0) == pdTRUE) {
                vTaskDelay(100 / portTICK_PERIOD_MS); //pega o objeto
                printf("[Robo_4] \tpegou da saida da [Maquina_%d]\n", prox+1);
                fflush(stdout);
                xSemaphoreGive(saida[prox][1]);
                vTaskDelay(600 / portTICK_PERIOD_MS); //leva até a saída da célula e solta
                printf("[Robo_4] \tdepositou na saida da celula\n");
                fflush(stdout);
                vTaskDelay(500 / portTICK_PERIOD_MS); //volta para a posição inicial

                int aux = prox; //inverte qual máquina terá a prioridade
                prox = ant;
                ant = aux;
            }
            else if (xSemaphoreTake(saida[ant][0], portMAX_DELAY) == pdTRUE) {
                vTaskDelay(100 / portTICK_PERIOD_MS); //pega o objeto
                printf("[Robo_4] \tpegou da saida da [Maquina_%d]\n", ant+1);
                fflush(stdout);
                xSemaphoreGive(saida[ant][1]);
                vTaskDelay(600 / portTICK_PERIOD_MS); //leva até a saída da célula e solta
                printf("[Robo_4] \tdepositou na saida da celula\n");
                fflush(stdout);
                vTaskDelay(500 / portTICK_PERIOD_MS); //volta para a posição inicial
            }

        }
    }
}

void maquina1(void* pvParameters) {
    (void)pvParameters; /* Just to remove compiler warnings. */

    while (1) {
        if (xSemaphoreTake(saida[0][1], portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(entrada[0][1], portMAX_DELAY) == pdTRUE) {
                printf("[Maquina_1] \tpegou da propria entrada\n");
                fflush(stdout);
                xSemaphoreGive(entrada[0][0]); //libera a entrada
                vTaskDelay(1500 / portTICK_PERIOD_MS); //processa o objeto
                printf("[Maquina_1] \tdepositou na propria saida\n");
                fflush(stdout);
				xSemaphoreGive(saida[0][0]); //indica item na saída
            }
        }
    }
}

void maquina2(void* pvParameters) {
    (void)pvParameters; /* Just to remove compiler warnings. */

    while (1) {
        if (xSemaphoreTake(saida[1][1], portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(entrada[1][1], portMAX_DELAY) == pdTRUE) {
                printf("[Maquina_2] \tpegou da propria entrada\n");
                fflush(stdout);
                xSemaphoreGive(entrada[1][0]); //libera a entrada
                vTaskDelay(1500 / portTICK_PERIOD_MS); //processa o objeto
                printf("[Maquina_2] \tdepositou na propria saida\n");
                fflush(stdout);
                xSemaphoreGive(saida[1][0]); //indica item na saída
				xSemaphoreGive(robo4SEM); //indica item disponível para o robô 4
            }
        }
    }
}

void maquina3(void* pvParameters) {
    (void)pvParameters; /* Just to remove compiler warnings. */

    while (1) {
        if (xSemaphoreTake(saida[2][1], portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(entrada[2][1], portMAX_DELAY) == pdTRUE) {
                printf("[Maquina_3] \tpegou da propria entrada\n");
                fflush(stdout);
                xSemaphoreGive(entrada[2][0]); //libera a entrada
                vTaskDelay(3000 / portTICK_PERIOD_MS); //processa o objeto
                printf("[Maquina_3] \tdepositou na propria saida\n");
                fflush(stdout);
                xSemaphoreGive(saida[2][0]); //indica item na saída
                xSemaphoreGive(robo4SEM); //indica item disponível para o robô 4
            }
        }
    }
}


int main( void )
{
    /* This demo uses heap_5.c, so start by defining some heap regions.  heap_5
     * is only used for test and example reasons.  Heap_4 is more appropriate.  See
     * http://www.freertos.org/a00111.html for an explanation. */
    prvInitialiseHeap();
    configASSERT( xTraceEnable(TRC_START) == TRC_SUCCESS );
    
    /* Use the cores that are not used by the FreeRTOS tasks for the Windows thread. */
    SetThreadAffinityMask( xWindowsKeyboardInputThreadHandle, ~0x01u );

    // Criando os semáforos
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            entrada[i][j] = xSemaphoreCreateBinary();
            saida[i][j] = xSemaphoreCreateBinary();
        }
    }
	robo4SEM = xSemaphoreCreateCounting(2, 0); // A qualquer momento pode ter 0, 1 ou 2 itens disponíveis para o robô 4

	// Liberando os semáforos de entrada e saída indicando que todos os buffers estão vazios
    for (int i = 0; i < 3; i++) {
        xSemaphoreGive(entrada[i][0]); 
        xSemaphoreGive(saida[i][1]);
    }

    /* create tasks */
    xTaskCreate(robo1, "Robo_1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(robo2, "Robo_2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(robo3, "Robo_3", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(robo4, "Robo_4", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    xTaskCreate(maquina1, "Maquina_1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(maquina2, "Maquina_2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(maquina3, "Maquina_3", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    
    /* start the scheduler */
    printf("Inicio da execucao:\n\n");
    fflush(stdout);

    vTaskStartScheduler();

    printf("Isso nao deveria aparecer\n");
    fflush(stdout);
    /* should never reach here! */
    for (;; );

    return 0;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
     * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
     * function that will get called if a call to pvPortMalloc() fails.
     * pvPortMalloc() is called internally by the kernel whenever a task, queue,
     * timer or semaphore is created.  It is also called by various parts of the
     * demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
     * size of the	heap available to pvPortMalloc() is defined by
     * configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
     * API function can be used to query the size of free heap space that remains
     * (although it does not provide information on how the remaining heap might be
     * fragmented).  See http://www.freertos.org/a00111.html for more
     * information. */
    vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void ) {}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
     * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     * function is called if a stack overflow is detected.  This function is
     * provided as an example only as stack overflow checking does not function
     * when running the FreeRTOS Windows port. */
    vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void ) {}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void ) {}
/*-----------------------------------------------------------*/

void vAssertCalled( unsigned long ulLine,
                    const char * const pcFileName )
{
    static BaseType_t xPrinted = pdFALSE;
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

    /* Called if an assertion passed to configASSERT() fails.  See
     * http://www.freertos.org/a00110.html#configASSERT for more information. */

    /* Parameters are not used. */
    ( void ) ulLine;
    ( void ) pcFileName;

    taskENTER_CRITICAL();
    {
        printf("ASSERT! Line %ld, file %s, GetLastError() %ld\r\n", ulLine, pcFileName, GetLastError());

        /* Stop the trace recording and save the trace. */
        ( void ) xTraceDisable();
        prvSaveTraceFile();

        /* Cause debugger break point if being debugged. */
        __debugbreak();

        /* You can step out of this function to debug the assertion by using
         * the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
         * value. */
        while( ulSetToNonZeroInDebuggerToContinue == 0 )
        {
            __asm {
                NOP
            };
            __asm {
                NOP
            };
        }

        /* Re-enable the trace recording. */
        ( void ) xTraceEnable( TRC_START );
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

static void prvSaveTraceFile( void )
{
    FILE * pxOutputFile;

    fopen_s( &pxOutputFile, mainTRACE_FILE_NAME, "wb" );

    if( pxOutputFile != NULL )
    {
        fwrite( RecorderDataPtr, sizeof( RecorderDataType ), 1, pxOutputFile );
        fclose( pxOutputFile );
        printf( "\r\nTrace output saved to %s\r\n\r\n", mainTRACE_FILE_NAME );
    }
    else
    {
        printf( "\r\nFailed to create trace dump file\r\n\r\n" );
    }
}
/*-----------------------------------------------------------*/

static void prvInitialiseHeap( void )
{
/* The Windows demo could create one large heap region, in which case it would
 * be appropriate to use heap_4.  However, purely for demonstration purposes,
 * heap_5 is used instead, so start by defining some heap regions.  No
 * initialisation is required when any other heap implementation is used.  See
 * http://www.freertos.org/a00111.html for more information.
 *
 * The xHeapRegions structure requires the regions to be defined in start address
 * order, so this just creates one big array, then populates the structure with
 * offsets into the array - with gaps in between and messy alignment just for test
 * purposes. */
    static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
    volatile uint32_t ulAdditionalOffset = 19; /* Just to prevent 'condition is always true' warnings in configASSERT(). */
    const HeapRegion_t xHeapRegions[] =
    {
        /* Start address with dummy offsets						Size */
        { ucHeap + 1,                                          mainREGION_1_SIZE },
        { ucHeap + 15 + mainREGION_1_SIZE,                     mainREGION_2_SIZE },
        { ucHeap + 19 + mainREGION_1_SIZE + mainREGION_2_SIZE, mainREGION_3_SIZE },
        { NULL,                                                0                 }
    };

    /* Sanity check that the sizes and offsets defined actually fit into the
     * array. */
    configASSERT( ( ulAdditionalOffset + mainREGION_1_SIZE + mainREGION_2_SIZE + mainREGION_3_SIZE ) < configTOTAL_HEAP_SIZE );

    /* Prevent compiler warnings when configASSERT() is not defined. */
    ( void ) ulAdditionalOffset;

    vPortDefineHeapRegions( xHeapRegions );
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
     * state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 * application must provide an implementation of vApplicationGetTimerTaskMemory()
 * to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/

/* The below code is used by the trace recorder for timing. */
static uint32_t ulEntryTime = 0;

void vTraceTimerReset( void )
{
    ulEntryTime = xTaskGetTickCount();
}

uint32_t uiTraceTimerGetFrequency( void )
{
    return configTICK_RATE_HZ;
}

uint32_t uiTraceTimerGetValue( void )
{
    return( xTaskGetTickCount() - ulEntryTime );
}
