#include "ctap2hid_packet.h"

#ifndef _PACKET_QUEUE_H_
#define _PACKET_QUEUE_H_

#define PACKET_QUEUE_LEN 5

typedef struct
{
    uint8_t head;
    uint8_t tail;
    uint8_t len;
    ctap2hid_packet_t packets[PACKET_QUEUE_LEN];
} packet_queue_t;

packet_queue_t pq_init(void);
void pq_push(packet_queue_t *q, ctap2hid_packet_t packet);
void pq_pop(packet_queue_t *q);
void pq_pop_n(packet_queue_t *q, uint8_t n);
ctap2hid_packet_t *pq_peek(packet_queue_t *q);
ctap2hid_packet_t *pq_peek_n(packet_queue_t *q, uint8_t n);
bool pq_is_empty(packet_queue_t *q);
bool pq_is_full(packet_queue_t *q);

#endif