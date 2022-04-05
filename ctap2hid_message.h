#include <stdlib.h>
#include "ctap2hid_packet.h"

#ifndef _CTAP2HID_MESSAGE_
#define _CTAP2HID_MESSAGE_

typedef struct
{
    uint32_t channel_id;
    uint8_t command_id;
    uint16_t payload_length;
    uint8_t *payload;
} ctap2hid_message_t;

typedef void writer_t(ctap2hid_packet_t *);
typedef void handler_t(ctap2hid_message_t *);
typedef ctap2hid_packet_t reader_t(void);

void write_message(ctap2hid_message_t *message, writer_t write);
bool read_message(reader_t read, handler_t handle);

#endif