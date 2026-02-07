#ifndef RENDERER_H
#define RENDERER_H

#include "game_math.h"
#include "bucket_array.h"

#ifdef RENDER_D3D11
    #include <d3d11_1.h>
#endif

#ifdef RENDER_D3D12
    #ifdef _GAMING_XBOX
        #include <d3d12_x.h>
        #define DX_IID IID_GRAPHICS_PPV_ARGS
    #else
        #include <d3d12.h>
        #define DX_IID IID_PPV_ARGS
    #endif
#endif

#ifdef RENDER_GL
    #include <GL/glew.h>
#endif

//
// DX11 Renderer Notes (see renderer_directx11.cpp)
//
// "Graphics Present" spikes may occur due to CPU/GPU synchronization stalls.
// Frame pacing and GPU driver behavior may vary across hardware, so behavior is not always consistent.
//
// GPU timestamp queries (e.g. via D3D11) are useful for narrowing down expensive rendering sections and can help correlate with "Graphics Present" stalls.
// see gpu_profiler.h
//
// Excessive buffer mapping or reuse—especially across multiple vertex or index buffers—can lead to performance issues due to buffer renaming or memory pressure.
// It's advisable to isolate dynamic GPU buffers per system (e.g. UI, Sprites, Text) to reduce contention and improve stability.
//
// Swap chain frame latency settings (e.g. max frame latency = 1) and use of frame waitable objects affect CPU-GPU synchronization behavior.
// These settings should be tuned carefully, as they can both reduce and introduce spikes depending on the GPU and driver.
//
// When VSync is disabled, ensure the swap chain allows tearing to avoid forced synchronization.
// Be aware that rendering at 60Hz on a 144Hz monitor may still cause subtle timing issues or stalls.
//
// NEXT STEPS:
//
// TODO(roger): For Present spikes, try out DXGI_PRESENT_DO_NOT_WAIT and drop frames.
//
// TODO(roger): Implement "Exclusive Fullscreen". See if this reduces the occassional Present spike on some machines.
//

#include "mesh_data.h"

#define RENDER_BUFFER_COUNT 2
#define VERTEX_COLOR_BUFFER_COUNT 1024
#define INDEX_COLOR_BUFFER_COUNT 4096
#define INITIAL_TILE_BUFFER_CAPACITY 5120000
#define RENDER_ITEM_LIMIT 128000
#define QUAD_VERTEX_COUNT 6

// Forward declare
struct RenderCommandGroup;
struct FontAsset;
struct Tilemap;

struct PerObjectBuffer {
	Matrix4 transform;
};

struct GizmoObjectBuffer {
	Matrix4 color;
};

enum GraphicsBufferType {
    VertexBuffer,
    IndexBuffer
};

enum GraphicsBufferUsage {
    StaticDraw,
    DynamicDraw
};

enum IndexBufferFormat {
    IndexFormat_None = 0,
    IndexFormat_U16,
    IndexFormat_U32
};

struct GpuBuffer {
    u32 count;
    u32 capacity;
    u32 stride;

    union {
    #ifdef RENDER_D3D11
        struct {
            ID3D11Buffer* buffer;
        } dx11;
    #endif

    #ifdef RENDER_D3D12
        struct {
            ID3D12Resource* resource;
            D3D12_VERTEX_BUFFER_VIEW vertexView;
            D3D12_INDEX_BUFFER_VIEW indexView;
        } dx12;
    #endif

    #ifdef RENDER_GL
        struct {
            GLuint buffer;
        } gl;
    #endif
    };

    void* mapped; //Is NULL when unmapped
    GraphicsBufferType type;
    GraphicsBufferUsage usage;
    IndexBufferFormat indexFormat; // Is NONE if type is VertexBuffer
};

enum VertexLayoutType {
    VertexLayout_None = 0,
    VertexLayout_XY,
    VertexLayout_XY_UV,
    VertexLayout_XY_RGBA,
    VertexLayout_XY_UV_RGBA,
    VertexLayout_XYZ,
    VertexLayout_XYZ_UV_RGBA,
    VertexLayout_Count
};

enum TextureFormat {
    TextureFormat_NIL = 0,
    TextureFormat_BGRA8_UNORM,
    TextureFormat_RGBA8_UNORM,
    TextureFormat_A8_UNORM,
    TextureFormat_BC7_UNORM,
};

struct Texture {
    u32 width;
    u32 height;
    TextureFormat format;
    bool isRenderTarget;
    int mipLevels;

    union {
    #ifdef RENDER_D3D11
        struct {
            ID3D11Texture2D* resource;
            ID3D11ShaderResourceView* srv;
            // 0 if the texture is NOT an render texture;
            ID3D11RenderTargetView* rtv;
        } dx11;
    #endif

    #ifdef RENDER_D3D12
        struct {
            ID3D12Resource* resource;
            D3D12_CPU_DESCRIPTOR_HANDLE cpu;
            D3D12_GPU_DESCRIPTOR_HANDLE gpu;
            // 0 if the texture is NOT an render texture;
            D3D12_CPU_DESCRIPTOR_HANDLE rtv;
            bool canSample;
        } dx12;
    #endif

    #ifdef RENDER_GL
        struct {
            GLuint tex;         // color texture
            GLuint fbo;         // framebuffer for render-to-texture (0 if not RT)
            GLuint ds;          // depth-stencil renderbuffer (0 if none)
        } gl;
    #endif
    };
};

struct ConstantBuffer {
    u32 paramIndex;

    union {
    #ifdef RENDER_D3D11
        struct { ID3D11Buffer* buffer; } dx11;
    #endif

    #ifdef RENDER_D3D12
        struct { ID3D12Resource* buffer; } dx12;
    #endif

    #ifdef RENDER_GL
        struct { GLuint buffer; } gl;
    #endif
    };
};

// Used to refer to a mesh stored in on the GPU.
struct MeshHandle {
    u32 vertexOffset;
    u32 vertexCount;
    GpuBuffer* vertexBuffer;
    u32 indexOffset;
    u32 indexCount;
    GpuBuffer* indexBuffer;
    Texture* texture;
};

struct SpriteMesh {
    int sortingLayer;
    int sortingOrder;
    u32 vertexCount;
    u32 indexCount;
    VertexUVColor* vertices;
    u16* indices;
    Texture* texture;
};

enum StencilMode {
    StencilMode_None = 0,
    StencilMode_Write,
    StencilMode_Read,
    StencilMode_Count
};

enum RasterizerType {
    Rasterizer_Default,
    Rasterizer_Wireframe
};

// TODO(roger): We only need these two types for now, but we can expand this based on our needs.
enum PrimitiveTopology {
    PrimitiveTopology_TriangleList,
    PrimitiveTopology_LineList,
    PrimitiveTopology_LineStrip,
};

enum ShaderBackend {
    ShaderBackend_GL,
    ShaderBackend_D3D,
    ShaderBackend_D3D_CSO,
};

struct ShaderBuffer {
    char* buffer;
    size_t size;
    size_t position;
};

struct ShaderProgram {
    ShaderBackend backend;
    union {
        struct {
            void* handle;
        } gl;

        struct {
            void* vertexShader;
            void* pixelShader;

            void* vsBytecode;
            void* psBytecode;

            ShaderBuffer vsCso;
            ShaderBuffer psCso;
        } d3d;
    };
};

struct VertexLayout {
    bool created;
    
    #ifdef RENDER_D3D11
        struct {
            ID3D11InputLayout* layout;
        } dx11;
    #endif

    #ifdef RENDER_GL
        struct {
            GLuint handle;
        } gl;
    #endif
};

enum BlendType {
    BlendType_Zero,
    BlendType_One,
    BlendType_Src_Alpha,
    BlendType_Inv_Src_Alpha,
};

enum BlendOp {
    BlendOp_Add,
    BlendOp_Subtract,
    BlendOp_Rev_Subtract,
    BlendOp_Min,
    BlendOp_Max
};

struct BlendDesc {
    bool enableBlend;
    
    BlendType srcBlend;
    BlendType destBlend;
    BlendOp blendOp;
    
    BlendType srcBlendAlpha;
    BlendType destBlendAlpha;
    BlendOp blendOpAlpha;
};

void BlendModeAlpha(BlendDesc* desc) {
    desc->enableBlend    = true;
    desc->srcBlend       = BlendType_Src_Alpha;
    desc->destBlend      = BlendType_Inv_Src_Alpha;
    desc->blendOp        = BlendOp_Add;
    desc->srcBlendAlpha  = BlendType_One;
    desc->destBlendAlpha = BlendType_Inv_Src_Alpha;
    desc->blendOpAlpha   = BlendOp_Add;
}

void BlendModePremultipliedAlpha(BlendDesc* desc) {
    desc->enableBlend    = true;
    desc->srcBlend       = BlendType_One;
    desc->destBlend      = BlendType_Inv_Src_Alpha;
    desc->blendOp        = BlendOp_Add;
    desc->srcBlendAlpha  = BlendType_One;
    desc->destBlendAlpha = BlendType_Inv_Src_Alpha;
    desc->blendOpAlpha   = BlendOp_Add;
}

void BlendModeOverwrite(BlendDesc* desc) {
    desc->enableBlend    = true;
    desc->srcBlend       = BlendType_One;
    desc->destBlend      = BlendType_Zero;
    desc->blendOp        = BlendOp_Add;
    desc->srcBlendAlpha  = BlendType_One;
    desc->destBlendAlpha = BlendType_Zero;
    desc->blendOpAlpha   = BlendOp_Add;
}

enum PipelineType {
    PIPELINE_NONE = 0,

    PIPELINE_SPRITE,
    PIPELINE_SPRITE_READ_MASK,
    PIPELINE_SPRITE_WRITE_MASK,

    PIPELINE_MESH,
    PIPELINE_MESH_READ_MASK,
    PIPELINE_MESH_WRITE_MASK,

    PIPELINE_TILEMAP,
    PIPELINE_TILEMAP_READ_MASK,
    PIPELINE_TILEMAP_WRITE_MASK,

    PIPELINE_LIGHTNING,

    PIPELINE_FONT,
    PIPELINE_GIZMO,
    PIPELINE_TEXTURE_MASK,
    PIPELINE_PRE_UI_FULLSCREEN,
    PIPELINE_POST_UI_FULLSCREEN,

    PIPELINE_COUNT
};

struct PipelineState {
    #ifdef RENDER_D3D11
        struct {
            ID3D11BlendState* blendState;
        } dx11;
    #endif

    #ifdef RENDER_D3D12
        struct {
            ID3D12RootSignature* rootSignature;
            ID3D12PipelineState* pso;
        } dx12;
    #endif

    PrimitiveTopology topology;
    VertexLayoutType vertexLayout;
    RasterizerType rasterizer;
    ShaderProgram* shader;
    BlendDesc blendDesc;
    StencilMode stencilMode;
};

enum SpriteRenderMode : u32 {
    SpriteRenderMode_Default = 0,
    SpriteRenderMode_Sliced,
    SpriteRenderMode_Count
};

struct SpriteRender {
    SpriteRenderMode mode;
    Texture* texture;
	Vector2 uvs[4];
	Vector2 sliceSize;
};

enum TextAlignment {
    TextAlignment_Center = 0,
    TextAlignment_Right,
    TextAlignment_Left
};

struct TextRender {
    int length;
    const char* str;
    TextAlignment alignment;
    FontAsset* font;
    int fontSize;
};

struct TextLine {
    u32 length;
    char* text;
    float width;
};

struct TextBatch {
    TextLine* lines;
    u32 lineCount;
};

enum RenderItemType {
    RenderItem_None = 0,
    RenderItem_Sprite,
    RenderItem_Spine,
    RenderItem_Tilemap,
    RenderItem_Text,
    RenderItem_SpriteMesh, // TODO(roger): Use Mesh?
    RenderItem_Mesh,
};

struct RenderItem {
    bool occupied;
    RenderItemType type;
    PipelineType pipeline;

    Matrix4 transform;
    Vector2 position;
    Vector2 scale;
    float rotation; //in radians
    int sortingLayer;
    int sortingOrder;
    Vector4 tint;

    union {
        Tilemap* tilemap;
        SpriteRender sprite;
        TextRender text;
        SpriteMesh spriteMesh; // TODO(roger): use mesh?
        MeshHandle mesh;
    };
};

RenderItem _renderItemStub;
RenderItem* RenderItemStub() {
    ZeroStruct(&_renderItemStub);
    return &_renderItemStub;
}

#define MAX_TILEMAP_TEXTURE_BATCH 6

struct TilemapRenderBatch {
    Texture* texture;
    u32 vertexOffset;
    u32 vertexCount;
};

struct TilemapRenderData {
    TilemapRenderBatch batches[MAX_TILEMAP_TEXTURE_BATCH];
    GpuBuffer vertexBuffer;
};

struct RenderTilemapCommand {
    u32 batchCount;
    GpuBuffer* vertexBuffer;
    TilemapRenderBatch batches[MAX_TILEMAP_TEXTURE_BATCH];
};

enum RenderCommandType {
    RenderCommand_None = 0,
    RenderCommand_DrawSpriteBatch,
    RenderCommand_DrawMesh,
    RenderCommand_UploadTilemapInstances,
    RenderCommand_DrawTilemap,
    RenderCommand_DrawTextBatch,
    RenderCommand_BeginScissor,
    RenderCommand_EndScissor
};

struct RenderCommand {
    RenderCommandType type;
    PipelineType pipeline;

    GpuBuffer* vertexBuffer;
    u32 vertexCount;
    u32 vertexOffset;

    Texture* texture;

    u32 constantBufferCount;
    ConstantBuffer* constantBuffers[3];

    Matrix4 worldTransform;

    union {
        RenderTilemapCommand drawTilemap;
        TextBatch drawTextBatch;
        MeshHandle drawMesh;

        struct {
            Vector2 point;
            Vector2 size;
        } scissor;
    };
};

RenderCommand _renderCommandStub;
RenderCommand* RenderCommandStub() {
    ZeroStruct(&_renderCommandStub);
    return &_renderCommandStub;
}

// NOTE(roger): A 'Render Layer' helps us organize render commands based on the 'sorting layer' of the scene being rendered.
struct RenderLayer {
    int sortingLayer;
    BucketArray<RenderCommand> renderCommands;
};

void InitializeGpuBuffer(GpuBuffer* buffer, u32 capacity, u32 stride, GraphicsBufferType type, GraphicsBufferUsage usage);
void ReallocGpuBuffer(GpuBuffer* buffer, u32 newCapacity);

thread_local u32 currentThreadId;
VertexLayout vertexLayouts[VertexLayout_Count];

//Renderer Backend function pointers
ShaderProgram (*LoadShader)(const char* name, VertexLayoutType layoutType);

void  (*BeginDrawing)();
void  (*EndDrawing)();
void  (*BeginTextureMode)(Texture* renderTexture);
void  (*EndTextureMode)();
void  (*DrawVertices)(u32 count, u32 offset);
void  (*DrawIndexedVertices)(u32 indexCount, u32 startIndex, int baseVertex);

void  (*CreateConstantBuffer)(u32 paramIndex, ConstantBuffer* constantBuffer, size_t size);
void  (*UpdateConstantBuffer)(ConstantBuffer* constantBuffer, void* data, size_t size);
void  (*BindConstantBuffers)(u32 startSlot, ConstantBuffer** constantBuffers, u32 count);

void  (*CreateGraphicsBuffer)(GpuBuffer* buffer, void* initialData);
void  (*ReleaseBuffer)(GpuBuffer* buffer);
void  (*MapBuffer)(GpuBuffer* buffer, bool writeDiscard);
void  (*UnmapBuffer)(GpuBuffer* buffer);
void  (*BindVertexBuffers)(GpuBuffer** buffers, u32 bufferCount);
void  (*BindIndexBuffer)(GpuBuffer* indexBuffer);

void  (*UnbindShaderTextures)(u32 count);
void  (*BindShaderTextures)(u32 startSlot, Texture** resources, u32 count);

bool  (*LoadTexture)(u8* data, Texture* texture);
void  (*UpdateTexture)(Texture* texture, u8* data);
void  (*CreateRenderTexture)(Texture* texture, u32 width, u32 height);
void  (*ReleaseTexture)(Texture* texture);
void* (*CreateTextureSampler)();
void  (*SetTextureSamplers)(u32 startSlot, void** samplers, u32 count);

void  (*SetScissor)(Vector2 point, Vector2 size);
void  (*CreatePipelineState)(PipelineState* state);
void  (*SetPipelineState)(PipelineState* state);

void  (*ClearBackground)(Vector4 color);
void  (*ResizeGraphics)(u32 width, u32 height, Texture* screenTextures, int screenTextureCount);
void  (*GraphicsPresent)();
// NOTE(roger): This needs to be called in near the start of the main loop (see win32_cultist, etc).
void  (*NewFrame)();

struct Sprite {
    Texture* texture;
    Vector2* uvs;
    Vector2 ppuScale;
    Vector2 pivot;
};

void SubmitSpriteForRender(Sprite sprite, Vector2 position, Vector4 tint, int sortingLayer, int sortingOrder);
void SubmitSpriteForRender(Sprite sprite, Vector2 position, float rotation, Vector2 scale, Vector4 tint, int sortingLayer, int sortingOrder);
void SubmitSpriteForRender(Texture* texture, Vector2* uvs, Matrix4 transform, Vector4 color, int sortingLayer, int sortingOrder, StencilMode stencilMode = StencilMode_None);
void SubmitSpriteSliced(Texture* texture, Matrix4 transform, Vector2 sliceSize, Vector4 color, int sortingLayer, int sortingOrder, StencilMode stencilMode = StencilMode_None);

#endif