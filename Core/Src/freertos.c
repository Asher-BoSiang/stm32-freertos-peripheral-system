/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sensor_data.h"
#include "stdio.h"
#include "string.h"
#include "w25q64.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define LOG_MAGIC 0xA5
#define FLASH_LOG_START_ADDR 0x000000
#define FLASH_LOG_SECTOR_SIZE 4096
typedef struct {
  uint8_t magic;
  uint32_t timestamp;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  uint8_t reserved[4]; // 4 bytes (Padding)
  uint8_t checksum;
  // total: 16 Bytes
} __attribute__((packed)) LogEntry_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t rx_data;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;

uint32_t s_log_addr = FLASH_LOG_START_ADDR;
bool s_logging_active = false;
uint32_t s_log_entry_count = 0;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for uart_rx_queue */
osMessageQueueId_t uart_rx_queueHandle;
const osMessageQueueAttr_t uart_rx_queue_attributes = {.name = "uart_rx_queue"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void SensorTask_Entry(void *argument);
void LoggerTask_Entry(void *argument);
uint8_t calc_entry_checksum(LogEntry_t *entry);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  SensorData_Init();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  const osThreadAttr_t sensorTask_attr = {
      .name = "SensorTask",
      .stack_size = 256 * 4,
      .priority = (osPriority_t)osPriorityAboveNormal,
  };
  osThreadNew(SensorTask_Entry, NULL, &sensorTask_attr);

  const osThreadAttr_t loggerTask_attr = {
      .name = "LoggerTask",
      .stack_size = 256 * 4,
      .priority = (osPriority_t)osPriorityBelowNormal,
  };
  osThreadNew(LoggerTask_Entry, NULL, &loggerTask_attr);
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of uart_rx_queue */
  uart_rx_queueHandle =
      osMessageQueueNew(64, sizeof(uint8_t), &uart_rx_queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle =
      osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
  /* USER CODE BEGIN StartDefaultTask */

  uint8_t boot_msg[] = "\r\n=== Welcome to STM32 RTOS Queue Shell ===\r\n> ";
  HAL_UART_Transmit(&huart1, boot_msg, strlen((char *)boot_msg), HAL_MAX_DELAY);
  HAL_UART_Receive_IT(&huart1, &rx_data, 1);

  // wake MPU6500
  uint8_t pwr_mgmt_1_reg = 0x6B;
  uint8_t wake_up_cmd = 0x00;

  // reset I2C1 prevent I2C lockup during hot-reload/reset
  HAL_I2C_DeInit(&hi2c1);
  osDelay(10);
  HAL_I2C_Init(&hi2c1);

  if (HAL_I2C_Mem_Write(&hi2c1, (0x68 << 1), pwr_mgmt_1_reg, 1, &wake_up_cmd, 1,
                        100) == HAL_OK) {
    uint8_t success_msg[] = "MPU6500 Wake up OK!\r\n> ";
    HAL_UART_Transmit(&huart1, success_msg, strlen((char *)success_msg),
                      HAL_MAX_DELAY);
  } else {
    uint8_t err_msg[] = "MPU6500 Wake up FAILED! (Check I2C wiring)\r\n> ";
    HAL_UART_Transmit(&huart1, err_msg, strlen((char *)err_msg), HAL_MAX_DELAY);
  }
  // Enable RX interrupt
  HAL_UART_Receive_IT(&huart1, &rx_data, 1);

  char local_buffer[50];
  uint8_t local_index = 0;
  uint8_t received_byte;

  /* Infinite loop */
  for (;;) {
    if (osMessageQueueGet(uart_rx_queueHandle, &received_byte, NULL,
                          osWaitForever) == osOK) {
      // Echo
      HAL_UART_Transmit(&huart1, &received_byte, 1, HAL_MAX_DELAY);

      if (received_byte == '\r' || received_byte == '\n') {
        local_buffer[local_index] = '\0';

        uint8_t newline[] = "\n";
        HAL_UART_Transmit(&huart1, newline, 1, HAL_MAX_DELAY);

        if (strcmp(local_buffer, "led on") == 0) {
          HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
          uint8_t res[] = "OK! LED is ON.\r\n";
          HAL_UART_Transmit(&huart1, res, strlen((char *)res), HAL_MAX_DELAY);
        } else if (strcmp(local_buffer, "led off") == 0) {
          HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
          uint8_t res[] = "OK! LED is OFF.\r\n";
          HAL_UART_Transmit(&huart1, res, strlen((char *)res), HAL_MAX_DELAY);
        } else if (strcmp(local_buffer, "get accel") == 0) {
          SensorData_t snapshot;

          if (SensorData_Read(&snapshot) == true) {
            char res[100];
            sprintf(res, "Accel X: %d, Y: %d, Z: %d | Tick: %lu\r\n",
                    snapshot.accel_x, snapshot.accel_y, snapshot.accel_z,
                    snapshot.timestamp);

            HAL_UART_Transmit(&huart1, (uint8_t *)res, strlen(res),
                              HAL_MAX_DELAY);
          } else {
            uint8_t err[] = "Error: Sensor data is busy!\r\n";
            HAL_UART_Transmit(&huart1, err, strlen((char *)err), HAL_MAX_DELAY);
          }
        } else if (strcmp(local_buffer, "log start") == 0) {
          W25Q64_SectorErase(FLASH_LOG_START_ADDR);
          s_log_addr = FLASH_LOG_START_ADDR;
          s_log_entry_count = 0;
          s_logging_active = true;
          char res[] = "Logging started! (1 record/sec)\r\n";
          HAL_UART_Transmit(&huart1, (uint8_t *)res, strlen(res),
                            HAL_MAX_DELAY);

        } else if (strcmp(local_buffer, "log stop") == 0) {
          s_logging_active = false;
          char res[60];
          snprintf(res, sizeof(res),
                   "Logging stopped. %lu entries recorded.\r\n",
                   s_log_entry_count);
          HAL_UART_Transmit(&huart1, (uint8_t *)res, strlen(res),
                            HAL_MAX_DELAY);

        } else if (strcmp(local_buffer, "log dump") == 0) {
          char header[] = "--- Log Dump Start ---\r\n";
          HAL_UART_Transmit(&huart1, (uint8_t *)header, strlen(header),
                            HAL_MAX_DELAY);

          uint32_t addr = FLASH_LOG_START_ADDR;
          for (uint32_t i = 0; i < s_log_entry_count; i++) {
            LogEntry_t entry;
            W25Q64_Read(addr, (uint8_t *)&entry, sizeof(LogEntry_t));

            char entry_str[100];
            if (entry.magic != LOG_MAGIC ||
                entry.checksum != calc_entry_checksum(&entry)) {
              snprintf(entry_str, sizeof(entry_str), "[%lu] CORRUPTED DATA\r\n",
                       i);
            } else {
              snprintf(entry_str, sizeof(entry_str),
                       "[%lu] T:%lu | X:%d Y:%d Z:%d\r\n", i, entry.timestamp,
                       entry.accel_x, entry.accel_y, entry.accel_z);
            }
            HAL_UART_Transmit(&huart1, (uint8_t *)entry_str, strlen(entry_str),
                              HAL_MAX_DELAY);
            addr += sizeof(LogEntry_t);
          }
          char footer[] = "--- Log Dump End ---\r\n";
          HAL_UART_Transmit(&huart1, (uint8_t *)footer, strlen(footer),
                            HAL_MAX_DELAY);

        } else if (strcmp(local_buffer, "log clear") == 0) {
          s_logging_active = false;
          W25Q64_SectorErase(FLASH_LOG_START_ADDR);
          s_log_addr = FLASH_LOG_START_ADDR;
          s_log_entry_count = 0;
          char res[] = "Flash Sector 0 cleared.\r\n";
          HAL_UART_Transmit(&huart1, (uint8_t *)res, strlen(res),
                            HAL_MAX_DELAY);
        } else if (local_index > 0) {
          uint8_t res[] = "Error: Unknown command!\r\n";
          HAL_UART_Transmit(&huart1, res, strlen((char *)res), HAL_MAX_DELAY);
        }

        uint8_t prompt[] = "> ";
        HAL_UART_Transmit(&huart1, prompt, 2, HAL_MAX_DELAY);

        local_index = 0;
      } else if (local_index < 49) {
        local_buffer[local_index] = received_byte;
        local_index++;
      }
    }
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void SensorTask_Entry(void *argument) {

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod = pdMS_TO_TICKS(200);

  for (;;) {
    uint8_t accel_data[6];

    if (HAL_I2C_Mem_Read(&hi2c1, (0x68 << 1), 0x3B, 1, accel_data, 6, 100) ==
        HAL_OK) {
      int16_t ax = (int16_t)((accel_data[0] << 8) | accel_data[1]);
      int16_t ay = (int16_t)((accel_data[2] << 8) | accel_data[3]);
      int16_t az = (int16_t)((accel_data[4] << 8) | accel_data[5]);

      SensorData_Update(ax, ay, az);
    }

    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {

    osMessageQueuePut(uart_rx_queueHandle, &rx_data, 0, 0);

    // ready for next character
    HAL_UART_Receive_IT(&huart1, &rx_data, 1);
  }
}

uint8_t calc_entry_checksum(LogEntry_t *entry) {
  uint8_t *p = (uint8_t *)entry;
  uint8_t cs = 0;
  for (size_t i = 0; i < sizeof(LogEntry_t) - 1; i++) {
    cs ^= p[i];
  }
  return cs;
}

void LoggerTask_Entry(void *argument) {
  for (;;) {
    if (s_logging_active) {
      SensorData_t snapshot;

      if (SensorData_Read(&snapshot) == true) {

        LogEntry_t entry = {.magic = LOG_MAGIC,
                            .timestamp = snapshot.timestamp,
                            .accel_x = snapshot.accel_x,
                            .accel_y = snapshot.accel_y,
                            .accel_z = snapshot.accel_z,
                            .reserved = {0xFF, 0xFF, 0xFF, 0xFF},
                            .checksum = 0};
        entry.checksum = calc_entry_checksum(&entry);

        W25Q64_PageProgram(s_log_addr, (uint8_t *)&entry, sizeof(LogEntry_t));
        s_log_addr += sizeof(LogEntry_t);
        s_log_entry_count++;

        if (s_log_addr + sizeof(LogEntry_t) >
            FLASH_LOG_START_ADDR + FLASH_LOG_SECTOR_SIZE) {
          s_logging_active = false;
          char msg[] =
              "\r\n[Warning] Flash Sector Full. Logging stopped.\r\n> ";
          HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg),
                            HAL_MAX_DELAY);
        }
      }
    }
    // 1 record/sec
    osDelay(1000);
  }
}
/* USER CODE END Application */
