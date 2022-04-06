#include "ctap2hid_message.h"

bool is_broadcast_message(ctap2hid_message_t message)
{
    return message.channel_id == 0xffffffff;
}

void write_message(ctap2hid_message_t *message, writer_t write)
{
    ctap2hid_packet_t packet = {
        .channel_id = message->channel_id,
        .init.command_id = message->command_id,
        .init.payload_length = SwapEndian_16(message->payload_length),
    };

    int len;

    for (len = 0; len < INIT_PAYLOAD_LENGTH; len++)
    {
        packet.init.payload[len] = len < message->payload_length ? message->payload[len] : 0;
    }

    int start = len;

    write(&packet);

    packet.cont.seq = 0;

    while (start < message->payload_length && packet.cont.seq <= 0x7F)
    {
        for (len = 0; len < CONT_PAYLOAD_LENGTH; len++)
        {
            packet.cont.payload[len] = start + len < message->payload_length ? message->payload[start + len] : 0;
        }

        write(&packet);

        start += len;

        packet.cont.seq++;
    }
}

bool read_message(reader_t read, message_handler_t handle_message, error_handler_t handle_error)
{
    ctap2hid_packet_t packet = read();

    if (!is_init_packet(packet))
    {
        handle_error(&packet, CTAPHID_ERR_MSG_TIMEOUT);
        return false;
    }

    ctap2hid_message_t message = {
        .channel_id = packet.channel_id,
        .command_id = packet.init.command_id & 0x7f,
        .payload_length = SwapEndian_16(packet.init.payload_length),
    };

    message.payload = malloc(message.payload_length);

    int position = 0;
    int size = MIN(message.payload_length - position, INIT_PAYLOAD_LENGTH);
    memcpy(message.payload, packet.init.payload, size);
    position += size;

    uint8_t seq = 0;

    while (position < message.payload_length)
    {
        packet = read();

        if (!is_cont_packet(packet) || seq != packet.cont.seq)
        {
            free(message.payload);
            handle_error(&packet, CTAPHID_ERR_INVALID_SEQ);
            return false;
        }

        size = MIN(message.payload_length - position, CONT_PAYLOAD_LENGTH);
        memcpy(message.payload + position, packet.cont.payload, size);
        position += size;

        seq++;
    }

    handle_message(&message);

    free(message.payload);

    return true;
}