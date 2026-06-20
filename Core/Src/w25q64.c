#include "w25q64.h"
#include "cmsis_os.h"

extern SPI_HandleTypeDef hspi1;

#define W25Q64_CS_LOW()                                                        \
  HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port, W25Q64_CS_Pin, GPIO_PIN_RESET)
#define W25Q64_CS_HIGH()                                                       \
  HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port, W25Q64_CS_Pin, GPIO_PIN_SET)

// Check Busy Flag
uint8_t W25Q64_ReadStatus(void) {
  uint8_t tx = CMD_READ_STATUS_REG1;
  uint8_t rx = 0;
  W25Q64_CS_LOW();
  HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, HAL_MAX_DELAY); // 送出指令
  HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, HAL_MAX_DELAY); // 讀取狀態
  W25Q64_CS_HIGH();
  return rx;
}

void W25Q64_WaitBusy(void) {
  while (W25Q64_ReadStatus() & 0x01) {
    osDelay(1);
  }
}

void W25Q64_WriteEnable(void) {
  uint8_t tx = CMD_WRITE_ENABLE;
  W25Q64_CS_LOW();
  HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);
  W25Q64_CS_HIGH();
}

void W25Q64_SectorErase(uint32_t addr) {
  uint8_t tx[4];
  tx[0] = CMD_SECTOR_ERASE;
  tx[1] = (addr >> 16) & 0xFF;
  tx[2] = (addr >> 8) & 0xFF;
  tx[3] = (addr) & 0xFF;

  W25Q64_WriteEnable();
  W25Q64_CS_LOW();
  HAL_SPI_Transmit(&hspi1, tx, 4, HAL_MAX_DELAY);
  W25Q64_CS_HIGH();
  W25Q64_WaitBusy(); // Wait for Erase
}

void W25Q64_PageProgram(uint32_t addr, uint8_t *data, uint16_t len) {
  uint8_t tx[4];
  tx[0] = CMD_PAGE_PROGRAM; // 0x02
  tx[1] = (addr >> 16) & 0xFF;
  tx[2] = (addr >> 8) & 0xFF;
  tx[3] = (addr) & 0xFF;

  W25Q64_WriteEnable();
  W25Q64_CS_LOW();
  HAL_SPI_Transmit(&hspi1, tx, 4, HAL_MAX_DELAY);
  HAL_SPI_Transmit(&hspi1, data, len, HAL_MAX_DELAY);
  W25Q64_CS_HIGH();
  W25Q64_WaitBusy();
}

void W25Q64_Read(uint32_t addr, uint8_t *buf, uint16_t len) {
  uint8_t tx[4];
  tx[0] = CMD_READ_DATA;
  tx[1] = (addr >> 16) & 0xFF;
  tx[2] = (addr >> 8) & 0xFF;
  tx[3] = (addr) & 0xFF;

  W25Q64_CS_LOW();
  HAL_SPI_Transmit(&hspi1, tx, 4, HAL_MAX_DELAY);
  HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY);
  W25Q64_CS_HIGH();
}