#include "ctap2hid_packet.h"

bool is_init_packet(ctap2hid_packet_t *packet)
{
    return packet->cont.seq & 0x80;
}

bool is_cont_packet(ctap2hid_packet_t *packet)
{
    return !is_init_packet(packet);
}