// TODO(roger): Remove from this file.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "inf_image.h"

struct Game {
    ConstantBuffer gameConstantBuffer;
    ConstantBuffer frameConstantBuffer;
    void* textureSampler;
    ShaderProgram simpleUvShader;
    ShaderProgram simpleShader;
    ShaderProgram simpleLightShader;
    GpuBuffer vertexBuffer;
    GpuBuffer indexBuffer;
    PipelineState meshPipeline;
    PipelineState wireframeMeshPipeline;
    Texture texture;
    
    int instancesToDraw;
    
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

struct Vector3Int {
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
    game.simpleShader = LoadShader("data/shaders/dx11/simple_line.fxh", VertexLayout_XYZ);
    game.simpleLightShader = LoadShader("data/shaders/dx11/simple_light.fxh", VertexLayout_XYZ_NORMAL);

    game.wireframeMeshPipeline.topology = PrimitiveTopology_LineList;
    game.wireframeMeshPipeline.vertexLayout = VertexLayout_XYZ;
    game.wireframeMeshPipeline.rasterizer = Rasterizer_Wireframe;
    game.wireframeMeshPipeline.shader = &game.simpleShader;
    BlendModeOverwrite(&game.wireframeMeshPipeline.blendDesc);
    game.wireframeMeshPipeline.stencilMode = StencilMode_None;
    CreatePipelineState(&game.wireframeMeshPipeline);
    
    game.meshPipeline.topology = PrimitiveTopology_TriangleList;
    game.meshPipeline.vertexLayout = VertexLayout_XYZ_NORMAL;
    game.meshPipeline.rasterizer = Rasterizer_Default;
    game.meshPipeline.shader = &game.simpleLightShader;
    game.meshPipeline.blendDesc.enableBlend = false;
    game.meshPipeline.stencilMode = StencilMode_None;
    CreatePipelineState(&game.meshPipeline);
    
    SvoImport svo = LoadSvo("data/svo/xyzrgb_statuette_8k.rsvo", ArenaAlloc);
    
    // TODO(roger): Use StaticDraw instead.
    InitializeGpuBuffer(&game.vertexBuffer, countOf(CubeVertices_XYZ_N) * 640000, sizeof(Vertex_XYZ_N), VertexBuffer, DynamicDraw);
    InitializeIndexBuffer(&game.indexBuffer, countOf(CubeTriIndices_XYZ_N), IndexFormat_U32, DynamicDraw);
    
    // TODO(roger): in Cultist, see if there is a function for appending data to a buffer (and resizing buffer when capacity is reached). should be something.
    MapBuffer(&game.vertexBuffer, true);
    
        Vector3Int** coordsAtLevel = ALLOC_ARRAY(TempAllocator, Vector3Int*, svo.topLevel + 1);
        coordsAtLevel[0] = ALLOC_ARRAY(TempAllocator, Vector3Int, 1);
        coordsAtLevel[0][0] = { 0, 0, 0 };

        for (int i = 0; i < 10; i++) {
            int parentCount = svo.nodesAtLevel[i]; 
            int childCount  = svo.nodesAtLevel[i + 1];
            coordsAtLevel[i + 1] = ALLOC_ARRAY(TempAllocator, Vector3Int, childCount);
            
            u32 w = 0;
            
            for (int p = 0; p < parentCount; p++) {
                Vector3Int pc = coordsAtLevel[i][p];
                u8 parentMask = svo.masksAtLevel[i][p];
                
                for (int child = 0; child < 8; child++) {
                    if (parentMask & (1u << child)) {
                        int xb = child & 1;
                        int yb = (child >> 1) & 1;
                        int zb = (child >> 2) & 1;
                        
                        coordsAtLevel[i + 1][w++] = { pc.x * 2 + xb, 
                                                      pc.y * 2 + yb, 
                                                      pc.z * 2 + zb };
                    }
                }    
            }
        }
        
        float rootSize = 8.0f;
        int lvl = 9;
        float voxelSize = rootSize / (1 << (lvl + 1));
        
        for (int i = 0; i < svo.nodesAtLevel[lvl]; ++i) {
            Vector3Int c = coordsAtLevel[lvl][i];
            
            size_t memSize = countOf(CubeVertices_XYZ_N) * sizeof(Vertex_XYZ_N);
            Vertex_XYZ_N* offset = (Vertex_XYZ_N*)((u8*)game.vertexBuffer.mapped + (game.instancesToDraw * memSize)); 
            memcpy(offset, CubeVertices_XYZ_N, memSize);
            
            for (int j = 0; j < countOf(CubeVertices_XYZ_N); j++) {
                offset[j].x = (CubeVertices_XYZ_N[j].x + c.x) * voxelSize;
                offset[j].y = (CubeVertices_XYZ_N[j].y + c.y) * voxelSize;
                offset[j].z = (CubeVertices_XYZ_N[j].z + c.z) * voxelSize;
            }
            game.vertexBuffer.count += countOf(CubeVertices_XYZ_N);
            game.instancesToDraw += 1;
        }
        
    UnmapBuffer(&game.vertexBuffer);
    
    MapBuffer(&game.indexBuffer, true);
        memcpy(game.indexBuffer.mapped, CubeTriIndices_XYZ_N, countOf(CubeTriIndices_XYZ_N) * sizeof(u32));
        game.indexBuffer.count += countOf(CubeTriIndices_XYZ_N);
    UnmapBuffer(&game.indexBuffer);
}

void TickGame() {
    
    // FPS is fixed to 60.
    float deltaTime = 1.0f / 60.0f;
    
    // TODO(roger): From Camera.
    local_persist float rot = 0;
    rot += pi/8.0f * deltaTime;
    
    Vector3 center = {1, 0,  1};
    Vector3 offset = {0, 2, -4};
    offset = RotateY(offset, rot);
    
    Vector3 eyePosition = center + offset;
    Vector3 forward = center - eyePosition;
    forward.y = 0;
    forward = Normalize(forward);
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
    
        BindConstantBuffers(0, constantBuffers, countOf(constantBuffers));
        
        SetPipelineState(&game.meshPipeline);
        BindVertexBuffers(vertexBuffers, 1);
        BindIndexBuffer(&game.indexBuffer);
        for (int i = 0; i < game.instancesToDraw; i++) {
            DrawIndexedVertices(countOf(CubeTriIndices_XYZ_N), 0, i * countOf(CubeVertices_XYZ_N));
        }

    EndDrawing();
    
    // TODO(roger): Rename to EndFrame()
    GraphicsPresent();
}