#include "boz_queue.h"

int
queue_serve(void *array, unsigned int num_elements, size_t element_size,
        struct queue_state *state, void *dest) {
    if (state->head == state->tail && !state->full) {
        return -1;
    }

    memcpy(dest, (char *) array + state->head * element_size, element_size);

    state->head++;
    if (state->head >= num_elements)
        state->head = 0;
    state->full = 0;

    return 0;
}

int
queue_add(void *array, unsigned int num_elements, size_t element_size,
        struct queue_state *state, const void *element) {
    if (state->full) {
        return -1;
    }

    memcpy((char *) array + state->tail * element_size, element, element_size);

    state->tail++;
    if (state->tail >= num_elements)
        state->tail = 0;
    if (state->head == state->tail)
        state->full = 1;

    return 0;
}

void
queue_clear(struct queue_state *state) {
    state->head = 0;
    state->tail = 0;
    state->full = 0;
}

int
queue_is_empty(struct queue_state *state) {
    return state->head == state->tail && !state->full;
}
