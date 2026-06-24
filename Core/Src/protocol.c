#include "protocol.h"
#include <string.h>

void Proto_Init(ProtoParser_t *parser) {
  parser->state = PROTO_STATE_WAIT_START;
  parser->cmd = 0;
  parser->length = 0;
  parser->data_index = 0;
  parser->checksum_calc = 0;
}

bool Proto_ProcessByte(ProtoParser_t *parser, uint8_t byte, Packet_t *out_pkt) {
  switch (parser->state) {

  case PROTO_STATE_WAIT_START:
    if (byte == 0xAA) {
      parser->state = PROTO_STATE_CMD;
      parser->checksum_calc = 0;
    }
    break;

  case PROTO_STATE_CMD:
    parser->cmd = byte;
    parser->checksum_calc ^= byte;
    parser->state = PROTO_STATE_LENGTH;
    break;

  case PROTO_STATE_LENGTH:
    parser->length = byte;
    parser->checksum_calc ^= byte;
    parser->data_index = 0;

    if (parser->length == 0) {
      parser->state = PROTO_STATE_CHECKSUM;
    } else if (parser->length > 16) {
      parser->state = PROTO_STATE_WAIT_START;
    } else {
      parser->state = PROTO_STATE_DATA;
    }
    break;

  case PROTO_STATE_DATA:
    parser->data[parser->data_index++] = byte;
    parser->checksum_calc ^= byte;

    if (parser->data_index >= parser->length) {
      parser->state = PROTO_STATE_CHECKSUM;
    }
    break;

  case PROTO_STATE_CHECKSUM:
    parser->state = PROTO_STATE_WAIT_START;

    if (byte == parser->checksum_calc) {
      out_pkt->cmd = parser->cmd;
      out_pkt->length = parser->length;
      memcpy(out_pkt->data, parser->data, parser->length);
      return true;
    }
    break;
  }

  return false;
}