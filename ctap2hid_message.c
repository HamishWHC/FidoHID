#include "ctap2hid_message.h"

bool is_broadcast_message(ctap2hid_message_t message)
{
    return message.channel_id == 0xffffffff;
}

void write_message_packets(ctap2hid_message_t *message, writer_t write)
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

uint8_t read_message_packets(ctap2hid_message_t *message, bool *err, packet_reader_t read, error_handler_t handle_error)
{
    uint8_t n = 0;

    ctap2hid_packet_t *packet = read(n);
    if (!packet || !is_init_packet(packet))
    {
        *err = true;
        return n;
    }

    message->channel_id = packet->channel_id;
    message->command_id = packet->init.command_id & 0x7f;
    message->payload_length = SwapEndian_16(packet->init.payload_length);
    message->payload = malloc(message->payload_length);

    int position = 0;
    int size = MIN(message->payload_length - position, INIT_PAYLOAD_LENGTH);
    memcpy(message->payload, packet->init.payload, size);
    position += size;

    n++;

    while (position < message->payload_length)
    {
        packet = read(n);

        if (!packet)
        {
            free(message->payload);
            return 0;
        }
        else if (!is_cont_packet(packet))
        {
            free(message->payload);
            *err = true;
            return n;
        }
        else if (n - 1 != packet->cont.seq)
        {
            handle_error(packet, CTAPHID_ERR_INVALID_SEQ);
            *err = true;
            return n;
        }

        size = MIN(message->payload_length - position, CONT_PAYLOAD_LENGTH);
        memcpy(message->payload + position, packet->cont.payload, size);
        position += size;

        n++;
    }

    return n;
}