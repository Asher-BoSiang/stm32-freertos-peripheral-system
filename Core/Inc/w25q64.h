#ifndef __W25Q64_H
#define __W25Q64_H

#include "main.h"

#define CMD_GET_JEDEC_ID 0x9F
#define CMD_WRITE_ENABLE 0x06
#define CMD_READ_STATUS_REG1 0x05
#define CMD_PAGE_PROGRAM 0x02
#define CMD_SECTOR_ERASE 0x20
#define CMD_READ_DATA 0x03

uint8_t W25Q64_ReadStatus(void);
void W25Q64_WaitBusy(void);
void W25Q64_WriteEnable(void);
void W25Q64_SectorErase(uint32_t addr);
void W25Q64_PageProgram(uint32_t addr, uint8_t *data, uint16_t len);
void W25Q64_Read(uint32_t addr, uint8_t *buf, uint16_t len);

#endif