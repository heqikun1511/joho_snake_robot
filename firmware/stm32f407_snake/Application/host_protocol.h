#ifndef HOST_PROTOCOL_H
#define HOST_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HOST_PROTOCOL_MAGIC_0       0x53U
#define HOST_PROTOCOL_MAGIC_1       0x52U
#define HOST_PROTOCOL_VERSION       1U
#define HOST_PROTOCOL_FRAME_SIZE    32U
#define HOST_PROTOCOL_PAYLOAD_SIZE  22U
#define HOST_PROTOCOL_MAX_JOINTS    8U

typedef enum {
    HOST_COMMAND_NOP = 0x00,
    HOST_COMMAND_PING = 0x01,
    HOST_COMMAND_ENABLE = 0x02,
    HOST_COMMAND_DISABLE = 0x03,
    HOST_COMMAND_SET_JOINTS = 0x04,
    HOST_COMMAND_GET_JOINTS = 0x05,
    HOST_COMMAND_GET_STATUS = 0x06,
    HOST_COMMAND_ESTOP = 0x07,
    HOST_RESPONSE_ACK = 0x80,
    HOST_RESPONSE_JOINTS = 0x81,
    HOST_RESPONSE_STATUS = 0x82,
    HOST_RESPONSE_ERROR = 0xFF
} HostMessageType;

typedef enum {
    HOST_PROTOCOL_OK = 0,
    HOST_PROTOCOL_BAD_ARGUMENT,
    HOST_PROTOCOL_BAD_MAGIC,
    HOST_PROTOCOL_BAD_VERSION,
    HOST_PROTOCOL_BAD_LENGTH,
    HOST_PROTOCOL_BAD_CRC
} HostProtocolResult;

typedef struct {
    uint8_t data[HOST_PROTOCOL_FRAME_SIZE];
} HostProtocolFrame;

void HostProtocol_InitFrame(HostProtocolFrame *frame,
                            HostMessageType type,
                            uint16_t sequence,
                            uint8_t joint_count);
bool HostProtocol_SetJointMilliradians(HostProtocolFrame *frame,
                                      uint8_t joint_index,
                                      int16_t position_mrad);
bool HostProtocol_GetJointMilliradians(const HostProtocolFrame *frame,
                                      uint8_t joint_index,
                                      int16_t *position_mrad);
void HostProtocol_Finalize(HostProtocolFrame *frame);
HostProtocolResult HostProtocol_Validate(const HostProtocolFrame *frame);
uint16_t HostProtocol_Crc16(const uint8_t *data, size_t length);

static inline HostMessageType HostProtocol_GetType(
    const HostProtocolFrame *frame)
{
    return (HostMessageType)frame->data[3];
}

static inline uint16_t HostProtocol_GetSequence(
    const HostProtocolFrame *frame)
{
    return (uint16_t)frame->data[4] |
           ((uint16_t)frame->data[5] << 8U);
}

static inline uint8_t HostProtocol_GetJointCount(
    const HostProtocolFrame *frame)
{
    return frame->data[6];
}

#endif
