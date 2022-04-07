#include "packet_queue.h"

packet_queue_t pq_init(void)
{
    packet_queue_t q = {
        .head = 0,
        .tail = 0,
        .len = 0,
        .packets = {},
    };
    return q;
}

void pq_push(packet_queue_t *q, ctap2hid_packet_t packet)
{
    q->len++;
    if (q->len > PACKET_QUEUE_LEN)
    {
        q->len--;
        return;
    }

    q->packets[q->tail] = packet;

    q->tail++;
    if (q->tail > PACKET_QUEUE_LEN)
    {
        q->tail = 0;
    }
}

void pq_pop(packet_queue_t *q)
{
    if (q->len == 0)
        return;

    q->head++;
    q->len--;
}

void pq_pop_n(packet_queue_t *q, uint8_t n)
{
    for (; q->len > 0 && n > 0; n--)
        pq_pop(q);
}

ctap2hid_packet_t *pq_peek_n(packet_queue_t *q, uint8_t n)
{
    if (q->len <= n)
        return NULL;
    return &q->packets[(q->head + n) % PACKET_QUEUE_LEN];
}

ctap2hid_packet_t *pq_peek(packet_queue_t *q)
{
    return pq_peek_n(q, 0);
}

bool pq_is_empty(packet_queue_t *q)
{
    return q->len == 0;
}

bool pq_is_full(packet_queue_t *q)
{
    return q->len == PACKET_QUEUE_LEN;
}