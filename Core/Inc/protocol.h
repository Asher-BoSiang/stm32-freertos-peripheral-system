#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  PROTO_STATE_WAIT_START,
  PROTO_STATE_CMD,
  PROTO_STATE_LENGTH,
  PROTO_STATE_DATA,
  PROTO_STATE_CHECKSUM
} ProtoState_t;

typedef struct {
  uint8_t cmd;
  uint8_t length;
  uint8_t data[16];
} Packet_t;

typedef struct {
  ProtoState_t state;
  uint8_t cmd;
  uint8_t length;
  uint8_t data[16];
  uint8_t data_index;
  uint8_t checksum_calc;
} ProtoParser_t;

#define CMD_GET_ACCEL 0x01
#define CMD_SET_LED 0x02
#define CMD_LOG_START 0x10
#define CMD_LOG_STOP 0x11
#define CMD_LOG_DUMP 0x12

void Proto_Init(ProtoParser_t *parser);
bool Proto_ProcessByte(ProtoParser_t *parser, uint8_t byte, Packet_t *out_pkt);

#endif /* __PROTOCOL_H */