#include <stdlib.h>
#include "ctap2hid_packet.h"
#include "ctaphid.h"

#ifndef _CTAP2HID_MESSAGE_
#define _CTAP2HID_MESSAGE_

typedef struct
{
    bool valid;
    uint32_t channel_id;
    uint8_t command_id;
    uint16_t payload_length;
    uint8_t *payload;
} ctap2hid_message_t;

typedef void writer_t(ctap2hid_packet_t *);
typedef void message_handler_t(ctap2hid_message_t *);
typedef ctap2hid_packet_t *packet_reader_t(uint8_t n);
typedef void error_handler_t(ctap2hid_packet_t *, uint8_t);

void write_message_packets(ctap2hid_message_t *message, writer_t write);
uint8_t read_message_packets(ctap2hid_message_t *message, bool *err, packet_reader_t read, error_handler_t handle_error);

#endif