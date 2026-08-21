#ifndef HOST_PROTOCOL_H
#define HOST_PROTOCOL_H

#include <stdint.h>

#define HOST_FRAME_HEADER_1       0xAA
#define HOST_FRAME_HEADER_2       0x55
#define HOST_PROTOCOL_VERSION     0x01

#define HOST_MAX_PAYLOAD_SIZE     64
#define HOST_FRAME_BODY_SIZE      (5 + HOST_MAX_PAYLOAD_SIZE + 2)

#define HOST_MSG_PING             0x01
#define HOST_MSG_PONG             0x81

#define HOST_MSG_SET_ONE_SERVO    0x10
#define HOST_MSG_COMMAND_ACK      0x90




