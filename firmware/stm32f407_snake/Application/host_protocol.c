#include "host_protocol.h"

#include <string.h>

#define HOST_PROTOCOL_CRC_OFFSET 30U

void HostProtocol_InitFrame(HostProtocolFrame *frame,
                            HostMessageType type,
                            uint16_t sequence,
                            uint8_t joint_count)
{
    if (frame == NULL) {
        return;
    }

    memset(frame->data, 0, sizeof(frame->data));
    frame->data[0] = HOST_PROTOCOL_MAGIC_0;
    frame->data[1] = HOST_PROTOCOL_MAGIC_1;
    frame->data[2] = HOST_PROTOCOL_VERSION;
    frame->data[3] = (uint8_t)type;
    frame->data[4] = (uint8_t)(sequence & 0xFFU);
    frame->data[5] = (uint8_t)(sequence >> 8U);
    frame->data[6] = joint_count;
}

bool HostProtocol_SetJointMilliradians(HostProtocolFrame *frame,
                                      uint8_t joint_index,
                                      int16_t position_mrad)
{
    uint8_t offset;

    if ((frame == NULL) || (joint_index >= HOST_PROTOCOL_MAX_JOINTS)) {
        return false;
    }

    offset = (uint8_t)(8U + (joint_index * 2U));
    frame->data[offset] = (uint8_t)((uint16_t)position_mrad & 0xFFU);
    frame->data[offset + 1U] = (uint8_t)((uint16_t)position_mrad >> 8U);
    return true;
}

bool HostProtocol_GetJointMilliradians(const HostProtocolFrame *frame,
                                      uint8_t joint_index,
                                      int16_t *position_mrad)
{
    uint8_t offset;
    uint16_t raw;

    if ((frame == NULL) || (position_mrad == NULL) ||
        (joint_index >= HOST_PROTOCOL_MAX_JOINTS)) {
        return false;
    }

    offset = (uint8_t)(8U + (joint_index * 2U));
    raw = (uint16_t)frame->data[offset] |
          ((uint16_t)frame->data[offset + 1U] << 8U);
    *position_mrad = (int16_t)raw;
    return true;
}

void HostProtocol_Finalize(HostProtocolFrame *frame)
{
    uint16_t crc;

    if (frame == NULL) {
        return;
    }

    crc = HostProtocol_Crc16(frame->data, HOST_PROTOCOL_CRC_OFFSET);
    frame->data[HOST_PROTOCOL_CRC_OFFSET] = (uint8_t)(crc & 0xFFU);
    frame->data[HOST_PROTOCOL_CRC_OFFSET + 1U] = (uint8_t)(crc >> 8U);
}

HostProtocolResult HostProtocol_Validate(const HostProtocolFrame *frame)
{
    uint16_t expected_crc;
    uint16_t received_crc;

    if (frame == NULL) {
        return HOST_PROTOCOL_BAD_ARGUMENT;
    }
    if ((frame->data[0] != HOST_PROTOCOL_MAGIC_0) ||
        (frame->data[1] != HOST_PROTOCOL_MAGIC_1)) {
        return HOST_PROTOCOL_BAD_MAGIC;
    }
    if (frame->data[2] != HOST_PROTOCOL_VERSION) {
        return HOST_PROTOCOL_BAD_VERSION;
    }
    if (frame->data[6] > HOST_PROTOCOL_MAX_JOINTS) {
        return HOST_PROTOCOL_BAD_LENGTH;
    }

    expected_crc = HostProtocol_Crc16(frame->data, HOST_PROTOCOL_CRC_OFFSET);
    received_crc = (uint16_t)frame->data[HOST_PROTOCOL_CRC_OFFSET] |
                   ((uint16_t)frame->data[HOST_PROTOCOL_CRC_OFFSET + 1U] << 8U);
    if (expected_crc != received_crc) {
        return HOST_PROTOCOL_BAD_CRC;
    }

    return HOST_PROTOCOL_OK;
}

uint16_t HostProtocol_Crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    size_t byte_index;
    uint8_t bit_index;

    if (data == NULL) {
        return 0U;
    }

    for (byte_index = 0U; byte_index < length; ++byte_index) {
        crc ^= (uint16_t)data[byte_index] << 8U;
        for (bit_index = 0U; bit_index < 8U; ++bit_index) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)((crc << 1U) ^ 0x1021U);
            } else {
                crc <<= 1U;
            }
        }
    }

    return crc;
}
