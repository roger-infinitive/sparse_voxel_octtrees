#ifndef _MEMORY_ARENA_H_
#define _MEMORY_ARENA_H_

#include <stdlib.h>
#include <string.h>

#define ARENA_ALLOC_ARRAY(allocator, type, count) \
    (type*)PushMemory((allocator), sizeof(type) * count);

struct MemoryArena {
    bool selfAllocated;
    size_t size;
    size_t used;
    void* buffer;
};

// Initializes a memory arena using malloc
void InitMemoryArena(MemoryArena* arena, size_t size) {
    arena->selfAllocated = true;
    arena->size = size;
    arena->used = 0;
    arena->buffer = malloc(size);
    ASSERT_ERROR(arena->buffer != NULL, "Failed to allocate memory for arena.");
    memset(arena->buffer, 0, size);
}

// Initialize a memory with a preallocated buffer.
void InitMemoryArenaWithBuffer(MemoryArena* arena, void* buffer, size_t size) {
    arena->selfAllocated = false;
    arena->size = size;
    arena->used = 0;
    arena->buffer = buffer;
    ASSERT_ERROR(arena->buffer != NULL, "An invalid buffer was provided to the arena.");
}

void* PushMemory(MemoryArena* arena, size_t size) {
    ASSERT_ERROR((arena->used + size) <= arena->size, "Memory overflow in arena.");
    void* result = (u8*)arena->buffer + arena->used;
    arena->used += size;
    return result;
}

// Resets the memory arena (does not deallocate memory)
void ResetMemoryArena(MemoryArena* arena) {
    arena->used = 0;
}

// Frees the memory associated with the arena
void FreeMemoryArena(MemoryArena* arena) {
    if (arena->selfAllocated) {
        free(arena->buffer);
    }
    arena->buffer = NULL;
    arena->used = 0;
    arena->size = 0;
}

struct TempArenaMemory {
    MemoryArena* arena;
    size_t used;
};

TempArenaMemory TempArenaMemoryBegin(MemoryArena* a) {
    TempArenaMemory temp;
    temp.arena = a;
    temp.used = a->used;
    return temp;
}

void TempArenaMemoryEnd(TempArenaMemory temp) {
    temp.arena->used = temp.used;
}

// Example usage:
/*
int main() {
    MemoryArena gameStateArena;
    InitMemoryArena(&gameStateArena, 1024 * 1024);  // Allocate 1 MB

    // Example: allocate a block of memory for 100 floats
    float* someFloats = (float*) PushMemory(&gameStateArena, 100 * sizeof(float));

    // Reset the entire arena
    ResetMemoryArena(&gameStateArena);

    // Clean up after we're done
    FreeMemoryArena(&gameStateArena);

    return 0;
}
*/

#endif //_MEMORY_ARENA_H_