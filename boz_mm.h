#ifndef _BOZ_MM_H
#define _BOZ_MM_H

#ifndef MM_TEST
typedef unsigned int size_t;
#endif

/* Allocate "size" bytes of memory from the pool and return a pointer to it. */
void *boz_mm_alloc(size_t size);

/* Free a chunk of memory previously returned by boz_mm_alloc(). */
void boz_mm_free(void *p);

size_t boz_mm_largest_free();
size_t boz_mm_total_size();

/*****************************************************************************
 * boz_mm_* functions below this comment are to be used by the Bozzard main
 * loop and API functions only, not by applications.
 ****************************************************************************/

/* Push the current used-chunks-list onto a stack, and create a new, empty
 * used-chunks-list, which subsequent chunks allocated by boz_mm_alloc() will
 * be added to. This is called by the Bozzard main loop when one app calls
 * another app. */
int boz_mm_push_context();

/* Free all chunks on the used-chunks-list, and pop the last used-chunks-list
 * from the stack. When an app returns, the Bozzard main loop calls this
 * function to free anything the app might have allocated, and returns control
 * to the calling app.
 *
 * This means an app is allowed to exit without explicitly freeing chunks it
 * allocated with boz_mm_alloc(). The app is entitled to assume that the main
 * loop will free the app's resources. */
int boz_mm_pop_context();

/* Initialise the memory manager with the given arena of memory and size. */
void boz_mm_init(char *arena, size_t arena_size);

/* alloc and free functions used by the Bozzard main loop and library functions
 * only. These allocate and free chunks in the same way as boz_mm_alloc
 * and boz_mm_free, but use a special private used-chunks-list which is never
 * affected by boz_mm_push_context() and boz_mm_pop_context(). */
void *boz_mm_main_alloc(size_t size);
void boz_mm_main_free(void *p);

#endif
