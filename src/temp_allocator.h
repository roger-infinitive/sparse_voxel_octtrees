#ifndef _TEMP_ALLOCATOR_H_
#define _TEMP_ALLOCATOR_H_

#include "memory_arena.h"

thread_local MemoryArena tempAllocator;

void* TempAlloc(size_t size) {
    return PushMemory(&tempAllocator, size);
}

void* TempRealloc(void* mem, size_t size) { return TempAlloc(size); }
void  TempFree(void* mem) { }

void InitTempAllocator(size_t size = MEGABYTES(256)) {
    ASSERT_ERROR(tempAllocator.size == 0, "Temp Allocator was already allocated for this thread!");
    InitMemoryArena(&tempAllocator, size);
}

void FreeTempAllocator() {
    FreeMemoryArena(&tempAllocator);
}

Allocator TempAllocator = {
    TempAlloc,
    TempRealloc,
    TempFree
};

#endif //_TEMP_ALLOCATOR_H_