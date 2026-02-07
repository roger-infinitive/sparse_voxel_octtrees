void InitializeGpuBuffer(GpuBuffer* buffer, u32 capacity, u32 stride, GraphicsBufferType type, GraphicsBufferUsage usage) {
    buffer->count = 0;
    buffer->capacity = capacity;
    buffer->stride = stride;
    buffer->type = type;
    buffer->usage = usage;

    CreateGraphicsBuffer(buffer, 0);
    
    if (buffer->type == IndexBuffer && buffer->indexFormat == IndexFormat_None) {
        ASSERT_DEBUG(false, "Index buffer was initialized without a format type. You should use InitializeIndexBuffer() or set the type before calling.");
    }
}

void InitializeIndexBuffer(GpuBuffer* buffer, u32 capacity, IndexBufferFormat indexFormat, GraphicsBufferUsage usage) {
    u32 stride = 0;
    switch (indexFormat) {
        case IndexFormat_U16: { stride = sizeof(u16); } break;
        case IndexFormat_U32: { stride = sizeof(u32); } break;
        default: { 
            ASSERT_ERROR(false, "Invalid index buffer type.");
            return;
        }
    }
    
    buffer->indexFormat = indexFormat;
    InitializeGpuBuffer(buffer, capacity, stride, IndexBuffer, usage);
}