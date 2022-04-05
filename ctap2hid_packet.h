#include "Descriptors.h"

#ifndef _CTAP2HID_PACKET_H_
#define _CTAP2HID_PACKET_H_

#define INIT_PAYLOAD_LENGTH FIDO_REPORT_SIZE - 7
#define CONT_PAYLOAD_LENGTH FIDO_REPORT_SIZE - 5

typedef struct
{
    uint32_t channel_id;
    union
    {
        struct
        {
            uint8_t command_id;
            uint16_t payload_length;
            uint8_t payload[INIT_PAYLOAD_LENGTH];
        } init;
        struct
        {
            uint8_t seq;
            uint8_t payload[CONT_PAYLOAD_LENGTH];
        } cont;
    };
} ctap2hid_packet_t;

bool is_init_packet(ctap2hid_packet_t packet);
bool is_cont_packet(ctap2hid_packet_t packet);

#endif