#ifdef MM_TEST
#include <stdlib.h>
#endif

#include "boz_mm.h"

#define boz_mm_align_type long
#define boz_mm_align sizeof(boz_mm_align_type)
#define BOZ_MM_LIST_STACK_SIZE 4

#define BOZ_MM_TAG_FREE 0x6472
#define BOZ_MM_TAG_USED 0x7573

char *boz_mm_arena = NULL;
boz_mm_size boz_mm_arena_size = 0;
byte crash_on_alloc_failure = 0;

extern void boz_crash(unsigned int);

struct boz_mm_header {
    union {
        struct {
            /* What kind of chunk - BOZ_MM_TAG_FREE or BOZ_MM_TAG_USED */
            int tag;

            /* Next chunk in the list */
            struct boz_mm_header *next;

            /* Previous chunk in the list */
            struct boz_mm_header *prev;

            /* Size of this chunk, including the header */
            boz_mm_size size;
        };
        boz_mm_align_type padding;
    };
};

struct boz_mm_header *boz_mm_free_list = NULL;
struct boz_mm_header *boz_mm_used_list = NULL;
struct boz_mm_header *boz_mm_main_used_list = NULL;

struct boz_mm_header *boz_mm_used_list_stack[BOZ_MM_LIST_STACK_SIZE];
int boz_mm_used_list_stack_ptr = 0;

#ifdef MM_TEST
#include <assert.h>
#define mm_assert(CONDITION, CODE) assert(CONDITION)
#else
#define mm_assert(CONDITION, CODE) { if (!(CONDITION)) boz_crash(CODE); }
#endif

static void mm_list_remove(struct boz_mm_header **startp, struct boz_mm_header *header) {
    if (header->prev) {
        mm_assert(header->prev->next == header, 3);
        header->prev->next = header->next;
    }
    else {
        *startp = header->next;
    }
    if (header->next) {
        mm_assert(header->next->prev == header, 4);
        header->next->prev = header->prev;
    }

    header->prev = NULL;
    header->next = NULL;
}


static void mm_list_add(struct boz_mm_header **startp, struct boz_mm_header *after, struct boz_mm_header *element) {
    /* Put "element" immediately after "after", or at the start of the list if
       "after" is NULL */
    if (after == NULL) {
        mm_assert(startp != NULL, 1);
        element->next = *startp;
        element->prev = NULL;
        if (*startp != NULL)
            (*startp)->prev = element;
        *startp = element;
    }
    else {
        element->next = after->next;
        element->prev = after;

        if (element->next)
            element->next->prev = element;
        element->prev->next = element;
    }
}

static void mm_list_insert(struct boz_mm_header **startp, struct boz_mm_header *header) {
    struct boz_mm_header *cur, *prev;

    prev = NULL;
    for (cur = *startp; cur != NULL; cur = cur->next) {
        if (header < cur) {
            /* header goes immediately before this one */
            break;
        }
        prev = cur;
    }

    mm_list_add(startp, prev, header);
}

static void mm_split_chunk(struct boz_mm_header *header, boz_mm_size size) {
    struct boz_mm_header *next;

    mm_assert(size % boz_mm_align == 0, 6);
    mm_assert(header->size >= size + sizeof(struct boz_mm_header), 2);

    next = (struct boz_mm_header *) (((char *) header) + size);
    next->tag = header->tag;
    next->size = header->size - size;
    header->size = size;

    mm_list_add(NULL, header, next);
}

static void mm_merge_with_next(struct boz_mm_header *header) {
    if (header->next != NULL) {
        header->size += header->next->size;
        mm_list_remove(NULL, header->next);
    }
}

static int are_chunks_contiguous(struct boz_mm_header *left, struct boz_mm_header *right) {
    return (((char *) left) + left->size == (char *) right);
}

#ifdef MM_TEST
static void mm_check_list_in_order(struct boz_mm_header *list) {
    while (list && list->next) {
        mm_assert(list < list->next, 10);
        list = list->next;
    }
}
#endif

void *boz_mm_alloc(boz_mm_size net_size) {
    /* Gross size: size including header and any alignment padding */
    boz_mm_size gross_size = net_size + sizeof(struct boz_mm_header);
    struct boz_mm_header *header;

    if (net_size == 0)
        return NULL;

    if (gross_size % boz_mm_align != 0)
        gross_size += boz_mm_align - (gross_size % boz_mm_align);

    mm_assert(gross_size % boz_mm_align == 0, 5);

    /* Find the first chunk in the free list that's at least the size we want */
    for (header = boz_mm_free_list; header; header = header->next) {
        if (header->size >= gross_size) 
            break;
    }

    if (header == NULL) {
        /* No available chunks */
#ifndef MM_TEST
        if (crash_on_alloc_failure)
            mm_assert(0, 15);
#endif
        return NULL;
    }

    mm_assert(header->tag == BOZ_MM_TAG_FREE, 7);

    /* If this chunk is big enough for our purposes with enough left over to
       hold another header and at least boz_mm_align bytes, split this chunk */
    if (header->size >= gross_size + sizeof(struct boz_mm_header) + boz_mm_align) {
        mm_split_chunk(header, gross_size);
    }

    /* Take this chunk off the free list */
    mm_list_remove(&boz_mm_free_list, header);

    header->tag = BOZ_MM_TAG_USED;

    /* Add it to the used list */
    mm_list_insert(&boz_mm_used_list, header);

#ifdef MM_TEST
    mm_check_list_in_order(boz_mm_free_list);
    mm_check_list_in_order(boz_mm_used_list);
#endif

    return (void *) (header + 1);
}

void boz_mm_free(void *ptr) {
    struct boz_mm_header *header;

    if (ptr == NULL)
        return;

    header = ((struct boz_mm_header *) ptr) - 1;

    mm_assert(header->tag == BOZ_MM_TAG_USED, 8);

    /* Remove this chunk from the allocated list */
    mm_list_remove(&boz_mm_used_list, header);

    header->tag = BOZ_MM_TAG_FREE;

    /* Insert this chunk into its appropriate place in the free list, which
       is sorted by address */
    mm_list_insert(&boz_mm_free_list, header);

    if (boz_mm_used_list)
        mm_assert(boz_mm_used_list->prev == NULL, 9);
    if (boz_mm_free_list)
        mm_assert(boz_mm_free_list->prev == NULL, 9);

    /* If this chunk is contiguous with the previous one, merge it with the
       previous one */
    if (header->prev && are_chunks_contiguous(header->prev, header)) {
        header = header->prev;
        mm_merge_with_next(header);
    }

    /* If this chunk is contiguous with the next one, merge it with the next */
    if (header->next && are_chunks_contiguous(header, header->next)) {
        mm_merge_with_next(header);
    }

#ifdef MM_TEST
    mm_check_list_in_order(boz_mm_free_list);
    mm_check_list_in_order(boz_mm_used_list);
#endif
}

void *boz_mm_main_alloc(boz_mm_size size) {
    struct boz_mm_header *old_used_list = boz_mm_used_list;
    void *p;
    boz_mm_used_list = boz_mm_main_used_list;
    p = boz_mm_alloc(size);
    boz_mm_used_list = old_used_list;
    return p;
}

void boz_mm_main_free(void *p) {
    struct boz_mm_header *old_used_list = boz_mm_used_list;
    boz_mm_used_list = boz_mm_main_used_list;
    boz_mm_free(p);
    boz_mm_used_list = old_used_list;
}


int boz_mm_push_context() {
    if (boz_mm_used_list_stack_ptr >= BOZ_MM_LIST_STACK_SIZE) {
        return -1;
    }
    boz_mm_used_list_stack[boz_mm_used_list_stack_ptr++] = boz_mm_used_list;
    boz_mm_used_list = NULL;
    return 0;
}

int boz_mm_pop_context() {
    if (boz_mm_used_list_stack_ptr <= 0) {
        return -1;
    }
    while (boz_mm_used_list) {
        boz_mm_free(boz_mm_used_list + 1);
    }
    boz_mm_used_list = boz_mm_used_list_stack[--boz_mm_used_list_stack_ptr];
    return 0;
}

boz_mm_size boz_mm_largest_free() {
    struct boz_mm_header *h;
    boz_mm_size largest = 0;

    for (h = boz_mm_free_list; h; h = h->next) {
        if (h->size - sizeof(struct boz_mm_header) > largest)
            largest = h->size - sizeof(struct boz_mm_header);
    }
    return largest;
}

boz_mm_size boz_mm_total_size() {
    return boz_mm_arena_size;
}

void boz_mm_init(char *arena, boz_mm_size arena_size) {
    boz_mm_arena = arena;
    boz_mm_arena_size = arena_size;
    boz_mm_used_list = NULL;
    boz_mm_free_list = (struct boz_mm_header *) boz_mm_arena;
    boz_mm_free_list->tag = BOZ_MM_TAG_FREE;
    boz_mm_free_list->prev = NULL;
    boz_mm_free_list->next = NULL;
    boz_mm_free_list->size = boz_mm_arena_size;
    boz_mm_used_list_stack_ptr = 0;
}
