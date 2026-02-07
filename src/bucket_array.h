#ifndef _BUCKET_ARRAY_H_
#define _BUCKET_ARRAY_H_

struct BucketArrayHandle {
    u32 bucketIndex;
    u32 index;
    u32 generation;
};

template <typename T>
struct BucketElement {
    bool isClaimed;
    BucketArrayHandle handle;
    BucketElement<T>* vacant;
    T data;
};

template <typename T>
struct Bucket {
    u32 count;
    BucketElement<T>* elements;
    
    T* operator[](u32 index) {
        return &elements[index].data;
    }
};

// TODO(roger): Rename count to bucketCount and elementCount to count.
template <typename T>
struct BucketArray {
    u32 count;
    Bucket<T>* buckets;
    u32 bucketCapacity;
    
    u32 elementCount;
    
    AllocFunc mem_alloc;
    FreeFunc mem_free;
    BucketElement<T>* vacant;
    
    void Initialize(u32 bucketCapacity, AllocFunc mem_alloc, FreeFunc mem_free);
    
    void Free() {
        for (u32 i = 0; i < count; i++) {
            Bucket<T>* bucket = &buckets[i];
            mem_free(bucket->elements);
            bucket->elements = 0;
        }
        mem_free(buckets);
        ZeroStruct(this);
    }
    
    T* GetData(BucketArrayHandle handle) {
        BucketElement<T>* element = GetElement(this, handle);
        if (element == 0) {
            return 0;
        }
        
        if (element->handle.generation != handle.generation) {
            return 0;
        }
        
        return &element->data;
    }
};

template <typename T>
Bucket<T>* InsertBucket(BucketArray<T>* bucketArray);

template <typename T>
void BucketArray<T>::Initialize(u32 bucketCapacity, AllocFunc mem_alloc, FreeFunc mem_free) {
    ZeroStruct(this);
    this->bucketCapacity = bucketCapacity;
    this->mem_alloc = mem_alloc;
    this->mem_free = mem_free;
    
    //TODO(roger): It would be nice to insert the first bucket here, but this currently causes problems with entity storage.
}

template <typename T>
Bucket<T>* InsertBucket(BucketArray<T>* bucketArray) {
    u64 size = (bucketArray->count + 1) * sizeof(Bucket<T>);
    Bucket<T>* buckets = (Bucket<T>*)bucketArray->mem_alloc(size);
    if (bucketArray->count > 0) {
        memcpy(buckets, bucketArray->buckets, bucketArray->count * sizeof(Bucket<T>));
        if (bucketArray->mem_free != 0) {
            bucketArray->mem_free(bucketArray->buckets);
        }
    }
    bucketArray->buckets = buckets;

    u32 bucketIndex = bucketArray->count;
    Bucket<T>* bucket = &buckets[bucketIndex];
    ZeroStruct(bucket);
    u64 elementsSize = sizeof(BucketElement<T>) * bucketArray->bucketCapacity;
    bucket->elements = (BucketElement<T>*)bucketArray->mem_alloc(elementsSize);
    
    for (u32 i = 0; i < bucketArray->bucketCapacity; i++) {
        BucketElement<T>* element = &bucket->elements[i];
        element->handle.bucketIndex = bucketIndex;
        element->handle.index = i;
        element->handle.generation = 0;
    }

    bucketArray->count++;
    return bucket;
}

template <typename T>
BucketElement<T>* GetElement(BucketArray<T>* bucketArray, BucketArrayHandle handle) {
    if (handle.bucketIndex >= bucketArray->count) {
        return 0;
    }

    Bucket<T>* bucket = &bucketArray->buckets[handle.bucketIndex];
    if (handle.index >= bucket->count) {
        return 0;    
    }
    
    BucketElement<T>* element = &bucket->elements[handle.index];
    if (element->handle.generation != handle.generation) {
        return 0;
    }
    
    return element;
}

template <typename T>
BucketArrayHandle AddElement(BucketArray<T>* bucketArray) {
    BucketElement<T>* element = bucketArray->vacant;
    if (element == 0) {
        u32 bucketIndex = bucketArray->count - 1;
        Bucket<T>* bucket = &bucketArray->buckets[bucketIndex];

        if (bucket->count == bucketArray->bucketCapacity) {
            bucket = InsertBucket<T>(bucketArray);
            bucketIndex++;
        }
        
        u32 index = bucket->count;
        element = &bucket->elements[index];
        bucket->count++;
    } else {
        bucketArray->vacant = element->vacant;
    }
    
    ASSERT_ERROR(element != 0, "Failed to find available element.");

    element->isClaimed = true;
    element->handle.generation++;
    T* data = &element->data;
    ZeroStruct(data);
    
    bucketArray->elementCount++;
    
    return element->handle;
}

template <typename T>
T* AddElementAndReturnData(BucketArray<T>* bucketArray) {
    BucketArrayHandle handle = AddElement(bucketArray);
    return &GetElement(bucketArray, handle)->data;
}

template <typename T> 
void RemoveElement(BucketArray<T>* bucketArray, BucketArrayHandle handle) {
    if (handle.generation == 0) {
        return;
    }

    BucketElement<T>* element = GetElement(bucketArray, handle);
    if (element == 0) {
        return;
    }

    element->handle.generation++;
    element->vacant = bucketArray->vacant;
    element->isClaimed = false;
    
    bucketArray->elementCount--;
    bucketArray->vacant = element;
}

template <typename T> 
void RemoveElement(BucketArray<T>* bucketArray, BucketElement<T>* element) {
    element->handle.generation++;
    element->vacant = bucketArray->vacant;
    element->isClaimed = false;
    
    bucketArray->elementCount--;
    bucketArray->vacant = element;
}

template <typename T> 
void ResetBucketArray(BucketArray<T>* bucketArray) {
    for (u32 i = 0; i < bucketArray->count; i++) {
        Bucket<T>* bucket = &bucketArray->buckets[i];
        for (u32 j = 0; j < bucket->count; j++) {
            BucketElement<T>* element = &bucket->elements[j];
            element->vacant = 0;
            element->isClaimed = false;
            memset(&element->data, 0, sizeof(T));
        }
        bucket->count = 0;
    }
    
    bucketArray->elementCount = 0;
    bucketArray->vacant = 0;
}

template <typename T>
struct BucketArrayIterator {
    BucketArray<T>* bucketArray;
    u32 bucketIndex;
    u32 index;
    BucketElement<T>* element;
    T* data;
};

template <typename T>
void InitBucketArrayIterator(BucketArrayIterator<T>* iterator, BucketArray<T>* bucketArray) {
    ZeroStruct(iterator);
    iterator->bucketArray = bucketArray;

    for (u32 i = 0; i < bucketArray->count; i++) {
        Bucket<T>* bucket = &bucketArray->buckets[i];
        for (u32 j = 0; j < bucket->count; j++) {
            BucketElement<T>* element = &bucket->elements[j];
            if (element->isClaimed) {
                iterator->bucketIndex = i;
                iterator->index = j;
                iterator->element = element;
                iterator->data = &element->data;
                return;
            }
        }
    }
}

template <typename T>
void AdvanceBucketArrayIterator(BucketArrayIterator<T>* iterator) {
    if (!iterator->bucketArray || !iterator->data) { 
        return;
    }

    BucketArray<T>* bucketArray = iterator->bucketArray;

    while (iterator->bucketIndex < bucketArray->count) {
        iterator->index++;
        
        Bucket<T>* currentBucket = &bucketArray->buckets[iterator->bucketIndex];
        
        while (iterator->index < currentBucket->count) {
            BucketElement<T>* element = &currentBucket->elements[iterator->index];
            if (element->isClaimed) {
                iterator->element = element;
                iterator->data = &element->data;
                return;
            }
            iterator->index++;
        }
        
        // move to the next bucket
        iterator->bucketIndex++;
        iterator->index = -1; // NOTE(roger): this will increment to 0 at the top of loop.
    }
    
    iterator->data = 0;
    iterator->element = 0;
}

template <typename T>
bool IsBucketArrayIteratorDone(BucketArrayIterator<T>* iterator) {
    return iterator->data == 0;
}

#define ForEachInBucketArray(it, bucketArray) \
    for (InitBucketArrayIterator(&it, bucketArray); !IsBucketArrayIteratorDone(&it); AdvanceBucketArrayIterator(&it))


template <typename T>
T* First(BucketArray<T>* bucketArray) {
    BucketArrayIterator<T> iterator;
    ForEachInBucketArray(iterator, bucketArray) {
        return iterator.data;
    }
    return 0;
}

template <typename T>
void CopyBucketArray(BucketArray<T>* dest, BucketArray<T>* src) {
    ASSERT_ERROR(dest->bucketCapacity == src->bucketCapacity, "NOT IMPLEMENTED: bucket arrays cannot be copied to bucket arrays with different bucket capacities.");
        
    if (dest->count > src->count) {
        for (u32 i = src->count; i < dest->count; i++) {
            Bucket<T>* bucket = &dest->buckets[i];
            dest->mem_free(bucket->elements);
            *bucket = {0};
        }
        
        size_t size = src->count * sizeof(Bucket<T>);
        void* buckets = dest->mem_alloc(size);
        memcpy(buckets, dest->buckets, size);
        dest->mem_free(dest->buckets);
        dest->buckets = (Bucket<T>*)buckets;
        
        dest->count = src->count;
        
        ASSERT_ERROR(dest->count <= src->count, "The destination bucket array cannot have more buckets than the src.");
    }
    
    while (dest->count < src->count) {
        InsertBucket(dest);
    }
    
    for (u32 i = 0; i < src->count; i++) {
        Bucket<T>* bucket     = &src->buckets[i];
        Bucket<T>* destBucket = &dest->buckets[i];
        
        destBucket->count = bucket->count;
        memcpy(destBucket->elements, bucket->elements, sizeof(BucketElement<T>) * src->bucketCapacity);
    }
    
    dest->vacant = src->vacant;
    dest->elementCount = src->elementCount;
}

#endif