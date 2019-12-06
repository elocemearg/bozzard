#include "boz_api.h"

#ifdef BOZ_SERIAL

#include <avr/pgmspace.h>

#define QUEUE_SIZE 32

struct queue {
    char buf[QUEUE_SIZE];
    byte head, tail, full;
};

struct queue out_queue;

int boz_serial_queue_space(struct queue *queue) {
    if (queue->full) {
        return 0;
    }
    else if (queue->head <= queue->tail) {
        return sizeof(out_queue) - (queue->tail - queue->head);
    }
    else {
        return queue->head - queue->tail;
    }
}

/* Send as many bytes as we can from out_queue to the serial port, without
   blocking. This is called by the main loop. */
void boz_serial_service_send(void) {
    struct queue *q = &out_queue;
    int max_write;
    byte queue_end;
    byte bytes_written = 0;

    /* If the queue is empty, there's nothing to send */
    if (q->head == q->tail && !q->full)
        return;

    /* How many bytes can we send without blocking? */
    max_write = Serial.availableForWrite();

    /* Send that many bytes, starting from out_queue_head. */
    while (bytes_written < max_write && (q->full || q->head != q->tail)) {
        if (q->head >= q->tail)
            queue_end = (byte) sizeof(q->buf);
        else
            queue_end = q->tail;

        Serial.write(q->buf + q->head, queue_end - q->head);
        bytes_written += queue_end - q->head;
        q->head = queue_end;
        q->full = 0;

        if (q->head >= sizeof(q->buf)) {
            q->head = 0;
        }
    }
}

int boz_serial_enqueue_data_out(char *buf, int length) {
    if (length <= 0)
        return 0;

    /* If we can shift any data out of our queue now without blocking, then
       do so. */
    boz_serial_service_send();

    return boz_serial_enqueue(&out_queue, buf, length);
}

int boz_serial_enqueue(struct queue *q, char *buf, int length) {
    if (length <= 0)
        return 0;

    /* If we don't have enough space for this message now, then fail to send
       it, rather than sending only half a message */
    if (length > boz_serial_queue_space(q))
        return -1;

    while (!q->full && length > 0) {
        int to_copy;
        if (q->tail < q->head) {
            to_copy = q->head - q->tail;
        }
        else {
            to_copy = sizeof(q->buf) - q->tail;
        }
        if (to_copy > length)
            to_copy = length;
        memcpy(q->buf + q->tail, buf, to_copy);
        q->tail += to_copy;
        if (q->tail >= sizeof(q->buf))
            q->tail = 0;
        buf += to_copy;
        length -= to_copy;
        if (q->head == q->tail)
            q->full = 1;
    }

    return 0;
}

void boz_serial_init(void) {
    Serial.begin(9600);
    memset(&out_queue, 0, sizeof(out_queue));
    //memset(&in_queue, 0, sizeof(in_queue));
}

#endif
