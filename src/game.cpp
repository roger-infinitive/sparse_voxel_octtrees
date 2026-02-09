#include "svo.cpp"

struct Game {
    ConstantBuffer gameConstantBuffer;
    ConstantBuffer frameConstantBuffer;
    ShaderProgram simpleLightShader;
    GpuBuffer vertexBuffer;
    GpuBuffer indexBuffer;
    PipelineState meshPipeline;
    
    MemoryArena memArena;
};

Game game;

void* ArenaAlloc(size_t size) {
    return PushMemory(&game.memArena, size);
}

Allocator ArenaAllocator {
    ArenaAlloc, 0, 0
};

void PackSvoMesh(SvoImport* svo, int lvl);

void RaycastSvo(SvoImport* svo, float rootScale, Vector3 rayStart, Vector3 rayDirection) {
    
    Vector3 v0 = {0, 0, 0};
    Vector3 v1 = {rootScale, rootScale, rootScale};
    
    Vector3 t_min;
    Vector3 t_max;
    
    float invDx = 0;
    float invDy = 0;
    float invDz = 0;
    
    // X slab
    if (Abs(rayDirection.x) < EPSILON) {
        if (rayStart.x < v0.x || rayStart.x > v1.x) {
            return;
        }
        t_min.x = -FLT_MAX; t_max.x = FLT_MAX;
    } else {
        invDx = 1.0f / rayDirection.x;
        float t0 = invDx * (v0.x - rayStart.x);                
        float t1 = invDx * (v1.x - rayStart.x);                
        t_min.x = Min(t0, t1);
        t_max.x = Max(t0, t1);
    }
    
    // Y slab
    if (Abs(rayDirection.y) < EPSILON) {
        if (rayStart.y < v0.y || rayStart.y > v1.y) {
            return;
        }
        t_min.y = -FLT_MAX; t_max.y = FLT_MAX;
    } else {
        invDy = 1.0f / rayDirection.y;
        float t0 = invDy * (v0.y - rayStart.y);                
        float t1 = invDy * (v1.y - rayStart.y);                
        t_min.y = Min(t0, t1);
        t_max.y = Max(t0, t1);
    }
    
    // Z slab
    if (Abs(rayDirection.z) < EPSILON) {
        if (rayStart.z < v0.z || rayStart.z > v1.z) {
            return;
        }
        t_min.z = -FLT_MAX; t_max.z = FLT_MAX;
    } else {
        invDz = 1.0f / rayDirection.z;
        float t0 = invDz * (v0.z - rayStart.z);                
        float t1 = invDz * (v1.z - rayStart.z);                
        t_min.z = Min(t0, t1);
        t_max.z = Max(t0, t1);
    }
    
    float t_enter = Max(t_min.x, Max(t_min.y, t_min.z));
    float t_exit  = Min(t_max.x, Min(t_max.y, t_max.z));
    float t = Max(t_enter, 0.0f);
    if (t_exit < t) {
        return;
    }
    
    Vector3 center = (v0 + v1) * 0.5f;
    Vector3 p = rayStart + (rayDirection * t); 
    
    int parent = 0;
    u8 idx = 0;

    Vector3 corner = v0;
    if (p.x >= center.x) { idx ^= 1; corner.x = rootScale * 0.5f; }   
    if (p.y >= center.y) { idx ^= 2; corner.y = rootScale * 0.5f; }   
    if (p.z >= center.z) { idx ^= 4; corner.z = rootScale * 0.5f; }   

    Vector3Int stepDir;
    stepDir.x = (rayDirection.x > 0) ? 1 : ((rayDirection.x < 0) ? -1 : 0);
    stepDir.y = (rayDirection.y > 0) ? 1 : ((rayDirection.y < 0) ? -1 : 0);
    stepDir.z = (rayDirection.z > 0) ? 1 : ((rayDirection.z < 0) ? -1 : 0);

    // TODO(roger): Add exit condition?
    while (true) {
        printf("traversing child idx %hhu in parent %d\n", idx, parent);
    
        u8 mask = svo->masksAtLevel[parent][0];
        if (mask & (1 << idx)) {
            // node occupied
            printf("node occupied\n");
        }
        
        // TODO(roger): Calculate scale based on level in octree
        float scale = rootScale * 0.5f;
        
        // TODO(roger): we need to track the current cube corner.
        Vector3 planes = corner;
        planes.x += stepDir.x * scale;
        planes.y += stepDir.y * scale;
        planes.z += stepDir.z * scale;
        
        float tx = invDx * (planes.x - rayStart.x);
        float ty = invDy * (planes.y - rayStart.y);
        float tz = invDz * (planes.z - rayStart.z);
        
        u8 stepMask = 0;
        if (tx < ty && tx < tz) {
            t = tx;
            stepMask = 1;
            corner.x += scale;
        } else if (ty < tz) {
            t = ty;
            stepMask = 2;
            corner.y += scale;
        } else {
            t = tz;
            stepMask = 4;
            corner.z += scale;
        }
        
        // idx & stepMask is 0 if in bounds, otherwise pop()
        if (idx & stepMask) {
            // TODO(roger): Need to implement Push() and Pop() to traverse children octants.
            // for now we exit as we are only traversing the root children.
            break;
        }
        
        printf("advance\n");
        idx ^= stepMask;
    }
}

void InitGame(const char* svoFilePath) {
    InitTempAllocator();
    InitMemoryArena(&game.memArena, MEGABYTES(256));
    
    CreateConstantBuffer(0, &game.gameConstantBuffer, sizeof(Matrix4));
    CreateConstantBuffer(1, &game.frameConstantBuffer, sizeof(Matrix4));
    
    Vector2 clientSize = GetClientSize();
    float aspect = clientSize.x / clientSize.y;
    Matrix4 projection = PerspectiveLH(aspect, DegreesToRadians(90), 0.1f, 100.0f);
    UpdateConstantBuffer(&game.gameConstantBuffer, &projection, sizeof(Matrix4));

    game.simpleLightShader = LoadShader("data/shaders/dx11/simple_light.fxh", VertexLayout_XYZ_NORMAL);

    game.meshPipeline.topology = PrimitiveTopology_TriangleList;
    game.meshPipeline.vertexLayout = VertexLayout_XYZ_NORMAL;
    game.meshPipeline.rasterizer = Rasterizer_Default;
    game.meshPipeline.shader = &game.simpleLightShader;
    game.meshPipeline.blendDesc.enableBlend = false;
    game.meshPipeline.stencilMode = StencilMode_None;
    CreatePipelineState(&game.meshPipeline);
    
    SvoImport svo = LoadSvo(svoFilePath, ArenaAlloc);
    
    // TODO(roger): Use StaticDraw instead.
    InitializeGpuBuffer(&game.vertexBuffer, 5120000, sizeof(Vertex_XYZ_N), VertexBuffer, DynamicDraw);
    InitializeIndexBuffer(&game.indexBuffer, 10240000, IndexFormat_U32, DynamicDraw);
    
    int lvl = 9;
    PackSvoMesh(&svo, lvl);
    
    RaycastSvo(&svo, 8.0f, {-1, -1, -1}, {10, 10, 10});
}

void TickGame() {
    
    // FPS is fixed to 60.
    float deltaTime = 1.0f / 60.0f;
    
    // TODO(roger): From Camera.
    local_persist float rot = 0;
    rot += pi/8.0f * deltaTime;
    
    Vector3 center = {1, 0,  1};
    Vector3 offset = {0, 1.5f, -3.5f};
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
        DrawIndexedVertices(game.indexBuffer.count, 0, 0);

    EndDrawing();
    
    // TODO(roger): Rename to EndFrame()
    GraphicsPresent();
}

void PackSvoMesh(SvoImport* svo, int lvl) {
    MapBuffer(&game.vertexBuffer, true);
    MapBuffer(&game.indexBuffer, true);
        
        Vector3Int** coordsAtLevel = ALLOC_ARRAY(TempAllocator, Vector3Int*, svo->topLevel + 1);
        coordsAtLevel[0] = ALLOC_ARRAY(TempAllocator, Vector3Int, 1);
        coordsAtLevel[0][0] = { 0, 0, 0 };

        for (int i = 0; i < lvl; i++) {
            int parentCount = svo->nodesAtLevel[i]; 
            int childCount  = svo->nodesAtLevel[i + 1];
            coordsAtLevel[i + 1] = ALLOC_ARRAY(TempAllocator, Vector3Int, childCount);
            
            u32 w = 0;
            
            for (int p = 0; p < parentCount; p++) {
                Vector3Int pc = coordsAtLevel[i][p];
                u8 parentMask = svo->masksAtLevel[i][p];
                
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
        
        // Compute first child for each level + parent index for faster IsFilled check.
        
        svo->firstChild = ALLOC_ARRAY(ArenaAllocator, u32*, lvl);
        for (int i = 0; i < lvl; ++i) {
            u32 parentCount = svo->nodesAtLevel[i];
            svo->firstChild[i] = ALLOC_ARRAY(ArenaAllocator, u32, parentCount);
            
            u32 run = 0;
            for (u32 p = 0; p < parentCount; ++p) {
                svo->firstChild[i][p] = run;
                run += Popcount8(svo->masksAtLevel[i][p]);
            }
            
            ASSERT_ERROR(run == svo->nodesAtLevel[i + 1], "child count mismatch!"); 
        }
        
        // Greedy Mesher
        
        float rootSize = 8.0f;
        float s = rootSize / (1 << (lvl + 1));
        
        for (int i = 0; i < svo->nodesAtLevel[lvl]; ++i) {
            Vector3Int c = coordsAtLevel[lvl][i];
            
            float x = c.x * s;
            float y = c.y * s;
            float z = c.z * s;
            
            if (!IsFilled(svo, lvl, Vector3Int{c.x + 1, c.y, c.z})) { 
                Vertex_XYZ_N verts[] = {
                    {s + x, y,     z,     1, 0, 0},
                    {s + x, s + y, z,     1, 0, 0},
                    {s + x, s + y, s + z, 1, 0, 0},
                    {s + x, y,     s + z, 1, 0, 0},
                };
                u32 indices[] = { 0 + game.vertexBuffer.count, 1 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 3 + game.vertexBuffer.count, 0 + game.vertexBuffer.count };
                
                AppendData(&game.vertexBuffer, verts, countOf(verts));
                AppendData(&game.indexBuffer, indices, countOf(indices));
            }
            
            if (!IsFilled(svo, lvl, Vector3Int{c.x - 1, c.y, c.z})) { 
                Vertex_XYZ_N verts[] = {
                    {x, y,     z,     -1, 0, 0},
                    {x, s + y, z,     -1, 0, 0},
                    {x, s + y, s + z, -1, 0, 0},
                    {x, y,     s + z, -1, 0, 0},
                };
                u32 indices[] = { 2 + game.vertexBuffer.count, 1 + game.vertexBuffer.count, 0 + game.vertexBuffer.count, 0 + game.vertexBuffer.count, 3 + game.vertexBuffer.count, 2 + game.vertexBuffer.count };
                
                AppendData(&game.vertexBuffer, verts, countOf(verts));
                AppendData(&game.indexBuffer, indices, countOf(indices));
            }
            
            if (!IsFilled(svo, lvl, Vector3Int{c.x, c.y + 1, c.z})) {
                Vertex_XYZ_N verts[] = {
                    {x,     s + y, z,     0, 1, 0},
                    {x,     s + y, s + z, 0, 1, 0},
                    {s + x, s + y, s + z, 0, 1, 0},
                    {s + x, s + y, z,     0, 1, 0},
                };
                u32 indices[] = { 0 + game.vertexBuffer.count, 1 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 3 + game.vertexBuffer.count, 0 + game.vertexBuffer.count };
                
                AppendData(&game.vertexBuffer, verts, countOf(verts));
                AppendData(&game.indexBuffer, indices, countOf(indices));
            }
            
            if (!IsFilled(svo, lvl, Vector3Int{c.x, c.y - 1, c.z})) { 
                Vertex_XYZ_N verts[] = {
                    {x,     y, z,     0, -1, 0},
                    {x,     y, s + z, 0, -1, 0},
                    {s + x, y, s + z, 0, -1, 0},
                    {s + x, y, z,     0, -1, 0},
                };
                u32 indices[] = { 0 + game.vertexBuffer.count, 3 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 1 + game.vertexBuffer.count, 0 + game.vertexBuffer.count };
                
                AppendData(&game.vertexBuffer, verts, countOf(verts));
                AppendData(&game.indexBuffer, indices, countOf(indices));
            }
            
            if (!IsFilled(svo, lvl, Vector3Int{c.x, c.y, c.z + 1})) { 
                Vertex_XYZ_N verts[] = {
                    {x,     y,     s + z, 0, 0, 1},
                    {x,     s + y, s + z, 0, 0, 1},
                    {s + x, s + y, s + z, 0, 0, 1},
                    {s + x, y,     s + z, 0, 0, 1},
                };
                u32 indices[] = { 0 + game.vertexBuffer.count, 3 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 1 + game.vertexBuffer.count, 0 + game.vertexBuffer.count };
                
                AppendData(&game.vertexBuffer, verts, countOf(verts));
                AppendData(&game.indexBuffer, indices, countOf(indices));
            }
            
            if (!IsFilled(svo, lvl, Vector3Int{c.x, c.y, c.z - 1})) { 
                Vertex_XYZ_N verts[] = {
                    {x,     y,     z, 0, 0, -1},
                    {x,     s + y, z, 0, 0, -1},
                    {s + x, s + y, z, 0, 0, -1},
                    {s + x, y,     z, 0, 0, -1},
                };
                u32 indices[] = { 0 + game.vertexBuffer.count, 1 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 2 + game.vertexBuffer.count, 3 + game.vertexBuffer.count, 0 + game.vertexBuffer.count };
                
                AppendData(&game.vertexBuffer, verts, countOf(verts));
                AppendData(&game.indexBuffer, indices, countOf(indices));
            }
        }
        
    UnmapBuffer(&game.indexBuffer);
    UnmapBuffer(&game.vertexBuffer);
}