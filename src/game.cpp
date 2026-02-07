// TODO(roger): Remove from this file.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "inf_image.h"

struct Game {
    // Render Data
    ConstantBuffer gameConstantBuffer;
    ConstantBuffer frameConstantBuffer;
    void* textureSampler;
    ShaderProgram simpleUvShader;
    ShaderProgram simpleLineShader;
    GpuBuffer vertexBuffer;
    GpuBuffer indexBuffer;
    PipelineState wireframeMeshPipeline;
    Texture texture;
    
    MemoryArena memArena;
};

Game game;

void* ArenaAlloc(size_t size) {
    return PushMemory(&game.memArena, size);
}

struct SvoImport {
    int topLevel;
    u32* nodesAtLevel;
    u8** masksAtLevel;
};

void VerifySvoPopCount(SvoImport* svo) {
    for (int lvl = 0; lvl < svo->topLevel; lvl++) {
        int count = svo->nodesAtLevel[lvl];
        int popcount = 0;
        for (int i = 0; i < count; i++) {
            u8 mask = svo->masksAtLevel[lvl][i];
            
            for (int j = 0; j < 8; j++) {
                if (mask & (1 << j)) {
                    popcount += 1;
                }
            }
        }
        ASSERT_ERROR(popcount == svo->nodesAtLevel[lvl + 1], "Incorrect popcount for SVO.\n");
        printf("popcount found/import - %d/%d\n", popcount, svo->nodesAtLevel[lvl + 1]);
    }
}

SvoImport LoadSvo(const char* filePath, AllocFunc alloc) { 
    SvoImport svo = {};
   
    TempArenaMemory arena = TempArenaMemoryBegin(&tempAllocator);
    
    MemoryBuffer mb = {};
    bool loaded = ReadEntireFileAndNullTerminate(filePath, &mb, TempAllocator);
    ASSERT_ERROR(loaded, "Failed to load SVO file: %s\n", filePath);

    char magicNumber[4];
    ReadBytes(&mb, magicNumber, 4);
    ASSERT_ERROR(memcmp(magicNumber, "RSVO", 4) == 0, "Wrong magic number for RSVO.");
    
    s32 version = ReadS32(&mb);
    ASSERT_ERROR(version == 1, "Unsupported version for RSVO.");
    
    s32 reserved0 = ReadS32(&mb);
    s32 reserved1 = ReadS32(&mb);

    svo.topLevel = ReadS32(&mb);

    svo.nodesAtLevel = (u32*)alloc(sizeof(u32) * (svo.topLevel + 1));
    for (int i = 0; i < svo.topLevel + 1; i++) {
        svo.nodesAtLevel[i] = ReadU32(&mb);
    }
    
    ASSERT_ERROR(svo.nodesAtLevel[0] == 1, "Top Level must only have 1 node.");
    
    svo.masksAtLevel = (u8**)alloc(sizeof(u8*) * (svo.topLevel + 1));
    svo.masksAtLevel[0] = 0;

    for (int i = 0; i < svo.topLevel; i++) {
        u32 count = svo.nodesAtLevel[i];
        u8* masks = (u8*)alloc(sizeof(u8) * count);
        ReadBytes(&mb, masks, count);
        svo.masksAtLevel[i] = masks;
    }
    
    ASSERT_ERROR(mb.position == mb.size, "Did not read entire file.");
    
    TempArenaMemoryEnd(arena);
    
    return svo;
}

struct SvoCoord {
    int x, y, z;
};

void InitGame() {
    InitTempAllocator();
    InitMemoryArena(&game.memArena, MEGABYTES(256));
    
    game.textureSampler = CreateTextureSampler();
    SetTextureSamplers(0, &game.textureSampler, 1);

    CreateConstantBuffer(0, &game.gameConstantBuffer, sizeof(Matrix4));
    CreateConstantBuffer(1, &game.frameConstantBuffer, sizeof(Matrix4));
    
    Vector2 clientSize = GetClientSize();
    float aspect = clientSize.x / clientSize.y;
    Matrix4 projection = PerspectiveLH(aspect, DegreesToRadians(90), 0.1f, 100.0f);
    UpdateConstantBuffer(&game.gameConstantBuffer, &projection, sizeof(Matrix4));

    game.simpleUvShader = LoadShader("data/shaders/dx11/simple_uv.fxh", VertexLayout_XY_UV_RGBA);
    game.simpleLineShader = LoadShader("data/shaders/dx11/simple_line.fxh", VertexLayout_XYZ);

    game.wireframeMeshPipeline.topology = PrimitiveTopology_LineList;
    game.wireframeMeshPipeline.vertexLayout = VertexLayout_XYZ;
    game.wireframeMeshPipeline.rasterizer = Rasterizer_Wireframe;
    game.wireframeMeshPipeline.shader = &game.simpleLineShader;
    BlendModePremultipliedAlpha(&game.wireframeMeshPipeline.blendDesc);
    game.wireframeMeshPipeline.stencilMode = StencilMode_None;
    CreatePipelineState(&game.wireframeMeshPipeline);
    
    SvoImport svo = LoadSvo("data/svo/xyzrgb_statuette_8k.rsvo", ArenaAlloc);

    int coordCount = 0;
    SvoCoord coords[4];
    
    int count = svo.nodesAtLevel[0];
    for (int i = 0; i < count; i++) {
        u8 mask = svo.masksAtLevel[0][i];
        
        for (int child = 0; child < 8; child++) {
            if (mask & (1u << child)) {
                int xBit = child & 1; 
                int yBit = (child >> 1) & 1; 
                int zBit = (child >> 2) & 1;
                
                coords[coordCount] = { xBit, yBit, zBit };
                coordCount += 1;
            }
        }
    }
    
    Vertex_XYZ lineVertices[] = {
        {0, 0, 0},
        {0, 1, 0},
        {1, 1, 0},
        {1, 0, 0},
        {0, 0, 1},
        {0, 1, 1},
        {1, 1, 1},
        {1, 0, 1},
    };
    
    u32 lineIndices[] = { 0, 1, 1, 2, 2, 3, 3, 0, 
                          0, 4, 1, 5, 2, 6, 3, 7,
                          4, 5, 5, 6, 6, 7, 7, 4 };
    
    // TODO(roger): Use StaticDraw instead.
    InitializeGpuBuffer(&game.vertexBuffer, countOf(lineVertices), sizeof(Vertex_XYZ), VertexBuffer, DynamicDraw);
    InitializeIndexBuffer(&game.indexBuffer, countOf(lineIndices), IndexFormat_U32, DynamicDraw);
    
    MapBuffer(&game.vertexBuffer, true);
        memcpy(game.vertexBuffer.mapped, lineVertices, countOf(lineVertices) * sizeof(Vertex_XYZ));
        game.vertexBuffer.count += countOf(lineVertices);
    UnmapBuffer(&game.vertexBuffer);
    
    MapBuffer(&game.indexBuffer, true);
        memcpy(game.indexBuffer.mapped, lineIndices, countOf(lineIndices) * sizeof(u32));
        game.indexBuffer.count += countOf(lineIndices);
    UnmapBuffer(&game.indexBuffer);
}

void TickGame() {
    
    // FPS is fixed to 60.
    float deltaTime = 1.0f / 60.0f;
    
    // TODO(roger): From Camera.
    local_persist Vector3 eyePosition = { 0, 0.5f, -5 };
    eyePosition = RotateY(eyePosition, pi/4 * deltaTime);  
    
    Vector3 forward = eyePosition * -1;
    Vector3 up = {0, 1, 0};

    Matrix4 view = LookToLH(eyePosition, forward, up);
    UpdateConstantBuffer(&game.frameConstantBuffer, &view, sizeof(Matrix4));
    
    // TODO(roger): Rename to BeginFrame()
    NewFrame();
    
    BeginDrawing();
        ClearBackground(Vector4{0.1f, 0.1f, 0.1f, 1.0f});
        ConstantBuffer* constantBuffers[2] = {
            &game.gameConstantBuffer,        
            &game.frameConstantBuffer,        
        };
        GpuBuffer* vertexBuffers[] = { &game.vertexBuffer };
        Texture* textures[] = { &game.texture };
    
        SetPipelineState(&game.wireframeMeshPipeline);
        BindConstantBuffers(0, constantBuffers, countOf(constantBuffers));
        BindVertexBuffers(vertexBuffers, 1);
        BindIndexBuffer(&game.indexBuffer);
        BindShaderTextures(0, textures, 1);
        DrawIndexedVertices(game.indexBuffer.count, 0, 0);
    EndDrawing();
    
    // TODO(roger): Rename to EndFrame()
    GraphicsPresent();
}