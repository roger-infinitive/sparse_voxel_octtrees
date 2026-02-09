#include "svo.cpp"

#define GIZMO_VERTEX_COUNT  640000
#define GIZMO_INDEX_COUNT  1280000

struct Game {
    ConstantBuffer gameConstantBuffer;
    ConstantBuffer frameConstantBuffer;
    
    ShaderProgram simpleShader;
    ShaderProgram simpleLightShader;
    
    GpuBuffer vertexBuffer;
    GpuBuffer indexBuffer;
    PipelineState meshPipeline;

    int gizmoVertexCount;
    Vertex_XYZ* gizmoVertices;
    int gizmoIndexCount;
    u32* gizmoIndices;
    GpuBuffer gizmoVertexBuffer;
    GpuBuffer gizmoIndexBuffer;
    PipelineState gizmoPipeline;
    
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

void DrawLine(Vector3 v0, Vector3 v1) {
    int vertexStart = game.gizmoVertexCount;
    game.gizmoVertices[vertexStart + 0].x = v0.x;
    game.gizmoVertices[vertexStart + 0].y = v0.y;
    game.gizmoVertices[vertexStart + 0].z = v0.z;
    game.gizmoVertices[vertexStart + 1].x = v1.x;
    game.gizmoVertices[vertexStart + 1].y = v1.y;
    game.gizmoVertices[vertexStart + 1].z = v1.z;
    game.gizmoVertexCount += 2;
    
    int indexStart = game.gizmoIndexCount;
    game.gizmoIndices[indexStart + 0] = vertexStart;
    game.gizmoIndices[indexStart + 1] = vertexStart + 1;
    game.gizmoIndexCount += 2;
}

void DrawAABB(Vector3 v0, Vector3 v1) {
    Vector3 size = v1 - v0;
    int vertexStart = game.gizmoVertexCount;
    int indexStart = game.gizmoIndexCount;
        
    memcpy(game.gizmoVertices + game.gizmoVertexCount, CubeVertices_XYZ, sizeof(CubeVertices_XYZ));
    game.gizmoVertexCount += countOf(CubeVertices_XYZ);

    for (int i = vertexStart; i < game.gizmoVertexCount; i++) {
        game.gizmoVertices[i].x *= size.x;    
        game.gizmoVertices[i].y *= size.y;    
        game.gizmoVertices[i].z *= size.z;
        game.gizmoVertices[i].x += v0.x;    
        game.gizmoVertices[i].y += v0.y;    
        game.gizmoVertices[i].z += v0.z;    
    }
    
    memcpy(game.gizmoIndices + game.gizmoIndexCount, CubeLineIndices_XYZ, sizeof(CubeLineIndices_XYZ));
    game.gizmoIndexCount += countOf(CubeLineIndices_XYZ);
    
    for (int i = indexStart; i < game.gizmoIndexCount; i++) {
        game.gizmoIndices[i] += vertexStart;
    }
}

struct SvoStackEntry {
    Vector3 corner;
    int parent;
    int mask_idx;
    u8 idx;
};

void RaycastSvo(SvoImport* svo, float rootScale, Vector3 rayStart, Vector3 rayDirection, int maxDepth) {
    
    Vector3 v0 = {0, 0, 0};
    Vector3 v1 = {rootScale, rootScale, rootScale};
    
    DrawLine(rayStart, rayStart + rayDirection);
    DrawAABB(v0, v1);
    
    Vector3 t_min;
    Vector3 t_max;
    
    Vector3Int stepDir;
    stepDir.x = (rayDirection.x > 0) ? 1 : ((rayDirection.x < 0) ? -1 : 0);
    stepDir.y = (rayDirection.y > 0) ? 1 : ((rayDirection.y < 0) ? -1 : 0);
    stepDir.z = (rayDirection.z > 0) ? 1 : ((rayDirection.z < 0) ? -1 : 0);
    
    if (Abs(rayDirection.x) < EPSILON) { rayDirection.x = EPSILON * (rayDirection.x < 0 ? -1 : 1); }
    if (Abs(rayDirection.y) < EPSILON) { rayDirection.y = EPSILON * (rayDirection.y < 0 ? -1 : 1); }
    if (Abs(rayDirection.z) < EPSILON) { rayDirection.z = EPSILON * (rayDirection.z < 0 ? -1 : 1); }
    
    float invDx = 1 / rayDirection.x;
    float invDy = 1 / rayDirection.y;
    float invDz = 1 / rayDirection.z;
    
    // X slab
    if (stepDir.x == 0) {
        if (rayStart.x < v0.x || rayStart.x > v1.x) {
            return;
        }
        t_min.x = -FLT_MAX; t_max.x = FLT_MAX;
    } else {
        float t0 = invDx * (v0.x - rayStart.x);                
        float t1 = invDx * (v1.x - rayStart.x);                
        t_min.x = Min(t0, t1);
        t_max.x = Max(t0, t1);
    }
    
    // Y slab
    if (stepDir.y == 0) {
        if (rayStart.y < v0.y || rayStart.y > v1.y) {
            return;
        }
        t_min.y = -FLT_MAX; t_max.y = FLT_MAX;
    } else {
        float t0 = invDy * (v0.y - rayStart.y);                
        float t1 = invDy * (v1.y - rayStart.y);                
        t_min.y = Min(t0, t1);
        t_max.y = Max(t0, t1);
    }
    
    // Z slab
    if (stepDir.z == 0) {
        if (rayStart.z < v0.z || rayStart.z > v1.z) {
            return;
        }
        t_min.z = -FLT_MAX; t_max.z = FLT_MAX;
    } else {
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
    
    // TODO(roger): use stack_index instead, and assert if greater than 32 (never should be unless the octree is extremely subdivided).
    SvoStackEntry stack[32];
    SvoStackEntry* current = &stack[0];
    *current = {};

    current->corner = v0;
    if (p.x >= center.x) { current->idx ^= 1; current->corner.x = rootScale * 0.5f; }   
    if (p.y >= center.y) { current->idx ^= 2; current->corner.y = rootScale * 0.5f; }   
    if (p.z >= center.z) { current->idx ^= 4; current->corner.z = rootScale * 0.5f; }   

    // TODO(roger): Calculate scale based on level in octree
    float scale = rootScale * 0.5f;
    
    // TODO(roger): debug only
    int indentCount = 0;
    char indents[32];
    memset(indents, 0, sizeof(indents));
    
    // TODO(roger): Add exit condition?
    while (true) {
        // TODO(roger): Test with rays along negative axes.
        Vector3 upperCorner = current->corner + Vector3{scale, scale, scale};
        DrawAABB(current->corner, upperCorner);
        printf("%straversing child idx %hhu in parent %d\n", indents, current->idx, current->parent);
    
        u8 mask = svo->masksAtLevel[current->parent][current->mask_idx];
        if (mask & (1u << current->idx) && current->parent < maxDepth) {
            printf("%spush\n", indents);
            indents[indentCount] = ' ';
            indentCount += 1;

            center = (current->corner + upperCorner) * 0.5f;
            
            // TODO(roger): Calculate scale based on parent instead.
            scale *= 0.5f;
            
            int parent_index = current->parent + 1;
            u8 beforeMask = mask & ((1u << current->idx) - 1u);
            
            Vector3 parent_corner = current->corner;
            current = &stack[parent_index];
            current->parent = parent_index;
            current->mask_idx = Popcount8(beforeMask);
            
            current->idx = 0;
            current->corner = parent_corner;
            if (p.x >= center.x) { current->idx ^= 1; current->corner.x += scale; }
            if (p.y >= center.y) { current->idx ^= 2; current->corner.y += scale; }
            if (p.z >= center.z) { current->idx ^= 4; current->corner.z += scale; }

            continue;
        }
        
        // TODO(roger): This will not work for rays going along a negative axis.
        float tx = invDx * (upperCorner.x - rayStart.x);
        float ty = invDy * (upperCorner.y - rayStart.y);
        float tz = invDz * (upperCorner.z - rayStart.z);
        
        u8 stepMask = 0;
        if (tx < ty && tx < tz) {
            t = tx;
            stepMask = 1;
        } else if (ty < tz) {
            t = ty;
            stepMask = 2;
        } else {
            t = tz;
            stepMask = 4;
        }
        
        // idx & stepMask is 0 if in bounds, otherwise pop()
        while (current->idx & stepMask) {
            if (current->parent == 0) {
                return;
            }
            
            indentCount -= 1;
            ASSERT_ERROR(indentCount >= 0, "cannot pop!");
            indents[indentCount] = 0;
            printf("%spop\n", indents);
            
            scale = rootScale / (1 << current->parent);
            current = &stack[current->parent - 1];
        }

        printf("%sadvance\n", indents);
        current->idx ^= stepMask;
        
        if (stepMask & 1) { 
            current->corner.x += scale; 
        } else if (stepMask & 2) { 
            current->corner.y += scale; 
        } else { 
            current->corner.z += scale; 
        } 
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

    game.simpleShader = LoadShader("data/shaders/dx11/simple.fxh", VertexLayout_XYZ);
    game.simpleLightShader = LoadShader("data/shaders/dx11/simple_light.fxh", VertexLayout_XYZ_NORMAL);

    {
        PipelineState* pipeline = &game.meshPipeline;
        pipeline->topology = PrimitiveTopology_TriangleList;
        pipeline->vertexLayout = VertexLayout_XYZ_NORMAL;
        pipeline->rasterizer = Rasterizer_Default;
        pipeline->shader = &game.simpleLightShader;
        pipeline->blendDesc.enableBlend = false;
        pipeline->stencilMode = StencilMode_None;
        CreatePipelineState(pipeline);
    }
    
    {
        PipelineState* pipeline = &game.gizmoPipeline; 
        pipeline->topology = PrimitiveTopology_LineList;
        pipeline->vertexLayout = VertexLayout_XYZ;
        pipeline->rasterizer = Rasterizer_Default;
        pipeline->shader = &game.simpleShader;
        pipeline->blendDesc.enableBlend = false;
        pipeline->stencilMode = StencilMode_None;
        CreatePipelineState(pipeline);
    } 
    
    // Allocate CPU side memory for gizmo mesh data
    game.gizmoVertices = ALLOC_ARRAY(ArenaAllocator, Vertex_XYZ, GIZMO_VERTEX_COUNT);
    game.gizmoIndices = ALLOC_ARRAY(ArenaAllocator, u32, GIZMO_INDEX_COUNT);
    
    SvoImport svo = LoadSvo(svoFilePath, ArenaAlloc);
    
    // TODO(roger): Use StaticDraw instead.
    InitializeGpuBuffer(&game.vertexBuffer, 5120000, sizeof(Vertex_XYZ_N), VertexBuffer, DynamicDraw);
    InitializeIndexBuffer(&game.indexBuffer, 10240000, IndexFormat_U32, DynamicDraw);
    
    InitializeGpuBuffer(&game.gizmoVertexBuffer, GIZMO_VERTEX_COUNT, sizeof(Vertex_XYZ), VertexBuffer, DynamicDraw);
    InitializeIndexBuffer(&game.gizmoIndexBuffer, GIZMO_INDEX_COUNT, IndexFormat_U32, DynamicDraw);
    
    int lvl = 9;
    PackSvoMesh(&svo, lvl);
    
    Vector3 rayStart     = { 2.15f, -1.0f, 1.15f };
    Vector3 rayDirection = { 0.0f, 8.0f, 10.0f };
    RaycastSvo(&svo, 8.0f, rayStart, rayDirection, 8);
}

void TickGame() {
    
    // FPS is fixed to 60.
    float deltaTime = 1.0f / 60.0f;
    
    // TODO(roger): From Camera.
    local_persist float rot = 0;
    rot += pi/8.0f * deltaTime;
    
    Vector3 center = {1, 0, 1};
    Vector3 offset = {0, 4.25f, -7.5f};
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
        BindConstantBuffers(0, constantBuffers, countOf(constantBuffers));
        
        // Draw Voxels
        
        GpuBuffer* vertexBuffers[] = { &game.vertexBuffer };
        SetPipelineState(&game.meshPipeline);
        BindVertexBuffers(vertexBuffers, countOf(vertexBuffers));
        BindIndexBuffer(&game.indexBuffer);
        DrawIndexedVertices(game.indexBuffer.count, 0, 0);
        
        // Draw Gizmos
        
        game.gizmoVertexBuffer.count = 0;
        MapBuffer(&game.gizmoVertexBuffer, true);
            AppendData(&game.gizmoVertexBuffer, game.gizmoVertices, game.gizmoVertexCount);
        UnmapBuffer(&game.gizmoVertexBuffer);
        
        game.gizmoVertexBuffer.count = 0;
        MapBuffer(&game.gizmoIndexBuffer, true);
            AppendData(&game.gizmoIndexBuffer, game.gizmoIndices, game.gizmoIndexCount);
        UnmapBuffer(&game.gizmoIndexBuffer);

        GpuBuffer* gizmoVertexBuffers[] = { &game.gizmoVertexBuffer };
        SetPipelineState(&game.gizmoPipeline);
        BindVertexBuffers(gizmoVertexBuffers, countOf(gizmoVertexBuffers));
        BindIndexBuffer(&game.gizmoIndexBuffer);
        DrawIndexedVertices(game.gizmoIndexBuffer.count, 0, 0);

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
        float s = rootSize / (1 << lvl);
        
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