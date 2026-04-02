#ifndef _FLUFFY_ARENA_H
#define _FLUFFY_ARENA_H

#include <stdint.h>

void* arena_alloc(void** alloc, uintptr_t size);
void arena_free(void* alloc);
uintptr_t arena_sizeof(void* alloc);

#endif
