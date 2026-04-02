#include "arena.h"
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char magic[4]; // ARENA_ALLOC_MAGIC
    uintptr_t size;
} arena_t;

static const char* ARENA_ALLOC_MAGIC = "ARN";

void* arena_alloc(void** alloc, uintptr_t size) {
    if (!*alloc) {
        *alloc = malloc(sizeof(arena_t));
        strncpy(((arena_t*)*alloc)->magic, ARENA_ALLOC_MAGIC, 3);
        ((arena_t*)*alloc)->size = 0;
        *alloc += sizeof(arena_t);
    }

    arena_t* arena = (arena_t*)(*alloc - sizeof(arena_t));
    if (strncmp(ARENA_ALLOC_MAGIC, arena->magic, 3) != 0) {
        printf("arena_alloc: invalid alloc address %p (%p)\n", *alloc, *alloc - sizeof(arena_t));
        raise(SIGABRT);
        return NULL;
    }

    arena->size += size;
    arena = realloc(arena, sizeof(arena_t) + arena->size);
    *alloc = arena + 1;
    return (void*)arena + sizeof(arena_t) + (arena->size - size);
}

void arena_free(void* alloc) {
    arena_t* arena = (arena_t*)(alloc - sizeof(arena_t));
    if (strncmp(ARENA_ALLOC_MAGIC, arena->magic, 3) != 0) {
        printf("arena_free: invalid alloc address %p (%p)\n", alloc, alloc - sizeof(arena_t));
        raise(SIGABRT);
        return;
    }
    strcpy(arena->magic, "\xaf\xaf\xaf");
    free(arena);
}

uintptr_t arena_sizeof(void* alloc) {
    arena_t* arena = (arena_t*)(alloc - sizeof(arena_t));
    if (strncmp(ARENA_ALLOC_MAGIC, arena->magic, 3) != 0) {
        printf("arena_sizeof: invalid alloc address %p (%p)\n", alloc, alloc - sizeof(arena_t));
        raise(SIGABRT);
        return 0;
    }

    return arena->size;
}
