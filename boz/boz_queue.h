#ifndef _BOZ_QUEUE_H
#define _BOZ_QUEUE_H

struct queue_state {
    unsigned short head, tail;
    unsigned char full;
};

int
queue_serve(void *array, unsigned int num_elements, size_t element_size,
        struct queue_state *state, void *dest);

int
queue_add(void *array, unsigned int num_elements, size_t element_size,
        struct queue_state *state, const void *element);

int
queue_is_empty(struct queue_state *state);

void
queue_clear(struct queue_state *state);

#endif
