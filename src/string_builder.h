#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include "utility.h"
#include <cstring>
#include <cstdarg>
#include "string_utility.h"

// TODO(roger): Add checks to prevent overflow.

struct StringBuilder {
    char* buffer;
    u64 position;
    u64 capacity;

    void Init(u64 capacity, Allocator allocator) {
        this->buffer = (char*)allocator.alloc(capacity);
        this->position = 0;
        this->capacity = capacity;
    }

    void Init(char* buffer, u64 capacity) {
        this->buffer = buffer;
        this->position = 0;
        this->capacity = capacity;
    }

    void Concat(const char c) {
        buffer[position] = c;
        position++;
        buffer[position] = '\0';
    }

    int Concat(const char* string) {
        size_t count = strlen(string);
        memcpy(buffer + position, string, count);
        position += count;
        buffer[position] = '\0';
        return count;
    }

    void Concat(void* data, u64 size) {
        if (size == 0) {
            return;
        }
    
        memcpy(buffer + position, data, size);
        position += size;
        buffer[position] = '\0';
    }
    
    void Newline() {
        buffer[position] = '\n';
        position++;
        buffer[position] = '\0';
    }

    int ConcatFormatted(const char* string, ...) {
        va_list args;
        va_start(args, string);
        int count = vsprintf(buffer + position, string, args);
        position += count;
        va_end(args);
        buffer[position] = '\0';
        return count;
    }

    int ConcatFormattedArgs(const char* string, va_list args) {
        int count = vsprintf(buffer + position, string, args);
        position += count;
        buffer[position] = '\0';
        return count;
    }

    char* CurrentPosition() {
        return buffer + position;
    }

    char* PushBytes(u64 count) {
        char* data = buffer + position;
        // PERFORMANCE(roger): This might be slow for granular writes.
        memset(data, 0, count);
        if (count == 0) {
            return data;
        }
    
        position += count;
        return data;
    }

    String ToString() {
        String str = {0};
        str.buffer = buffer;
        str.length = position;
        return str;
    }
};

#endif //STRING_BUILDER_H