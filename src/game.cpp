#include "input_common.h"

#include "svo.cpp"
#include "input_common.cpp"
#include "camera.cpp"

#include "game.h"

void InitGame(const char* svoFilePath) {
    InitTempAllocator();
    InitMemoryArena(&game.memArena, MEGABYTES(256));
    
    CreateConstantBuffer(0, &game.gameConstantBuffer, sizeof(Matrix4));
    CreateConstantBuffer(1, &game.frameConstantBuffer, sizeof(Matrix4));
    
    Vector2 clientSize = GetClientSize();
    float aspect = clientSize.x / clientSize.y;
    Matrix4 projection = PerspectiveLH(aspect, DegreesToRadians(90), 0.001f, 100.0f);
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
    
    game.svo = LoadSvo(svoFilePath, ArenaAlloc);
    
    // TODO(roger): Use StaticDraw instead.
    InitializeGpuBuffer(&game.vertexBuffer, 5120000, sizeof(Vertex_XYZ_N), VertexBuffer, DynamicDraw);
    InitializeIndexBuffer(&game.indexBuffer, 10240000, IndexFormat_U32, DynamicDraw);
    
    InitializeGpuBuffer(&game.gizmoVertexBuffer, GIZMO_VERTEX_COUNT, sizeof(Vertex_XYZ), VertexBuffer, DynamicDraw);
    InitializeIndexBuffer(&game.gizmoIndexBuffer, GIZMO_INDEX_COUNT, IndexFormat_U32, DynamicDraw);
    
    int lvl = 9;
    PackSvoMesh(&game.svo, lvl);
}

void TickGame() {
    // FPS is fixed to 60.
    float deltaTime = 1.0f / 60.0f;

    Vector2 mouseDelta = {};
    if (isWindowActive) {
        Vector2 screenSize = GetClientSize();
        mouseDelta = GetMousePosition() - (screenSize * 0.5f);
        mouseDelta.x /= screenSize.x;
        mouseDelta.y /= screenSize.y;
        if (Abs(mouseDelta.x) < 0.001f) mouseDelta.x = 0;
        if (Abs(mouseDelta.y) < 0.001f) mouseDelta.y = 0;
    }
    
    Matrix4 view = TickCamera(&game.camera, mouseDelta, deltaTime);
    UpdateConstantBuffer(&game.frameConstantBuffer, &view, sizeof(Matrix4));

    if (IsInputPressed(KEY_M)) {
        game.hide_model = !game.hide_model;        
    }
    
    if (IsInputPressed(KEY_R)) {
        float cy = cosf(game.camera.yaw);
        float sy = sinf(game.camera.yaw);
        float cp = cosf(game.camera.pitch);
        float sp = sinf(game.camera.pitch);
        Vector3 forward = Normalize(Vector3{sy * cp, sp, cy * cp});
        
        forward *= 12.0f;
        RaycastSvo(&game.svo, 8.0f, game.camera.position, forward, 8);
    }
    
    if (IsInputPressed(KEY_C)) {
        game.gizmoVertexCount = 0;
        game.gizmoIndexCount  = 0;
    }
    
    if (IsInputPressed(KEY_ESCAPE)) {
        QuitGame();
    }
    
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
        if (!game.hide_model) {
            GpuBuffer* vertexBuffers[] = { &game.vertexBuffer };
            SetPipelineState(&game.meshPipeline);
            BindVertexBuffers(vertexBuffers, countOf(vertexBuffers));
            BindIndexBuffer(&game.indexBuffer);
            DrawIndexedVertices(game.indexBuffer.count, 0, 0);
        }
        
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
    
    if (isWindowActive) {
        HideOsCursor();
        CenterCursorInWindow();
    } else {
        ShowOsCursor();
    }
    FlushInput();
}

void PackSvoMesh(SvoImport* svo, int lvl) {
    TempArenaMemory tempArena = TempArenaMemoryBegin(&tempAllocator);

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
    
    TempArenaMemoryEnd(tempArena);
}

struct SvoStackEntry {
    Vector3 corner;
    int mask_idx;
    s8 idx;
};

void RaycastSvo(SvoImport* svo, float rootScale, Vector3 rayStart, Vector3 rayDirection, int maxDepth) {
    TempArenaMemory tempArena = TempArenaMemoryBegin(&tempAllocator);
    
    Vector3 v0 = {0, 0, 0};
    Vector3 v1 = {rootScale, rootScale, rootScale};
    
    DrawLine(rayStart, rayStart + rayDirection);
    
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
    
    Vector3 t_min = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    Vector3 t_max = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
    
    // X slab
    if (stepDir.x == 0) {
        if (rayStart.x < v0.x || rayStart.x > v1.x) {
            return;
        }
    } else {
        t_min.x = invDx * (v0.x - rayStart.x);                
        t_max.x = invDx * (v1.x - rayStart.x);
        if (t_max.x < t_min.x) { 
            SWAP(t_max.x, t_min.x);
        }
    }
    
    // Y slab
    if (stepDir.y == 0) {
        if (rayStart.y < v0.y || rayStart.y > v1.y) {
            return;
        }
    } else {
        t_min.y = invDy * (v0.y - rayStart.y);                
        t_max.y = invDy * (v1.y - rayStart.y);                
        if (t_max.y < t_min.y) {
            SWAP(t_max.y, t_min.y);
        }
    }
    
    // Z slab
    if (stepDir.z == 0) {
        if (rayStart.z < v0.z || rayStart.z > v1.z) {
            return;
        }
    } else {
        t_min.z = invDz * (v0.z - rayStart.z);                
        t_max.z = invDz * (v1.z - rayStart.z);                
        if (t_max.z < t_min.z) {
            SWAP(t_max.z, t_min.z);
        }
    }
    
    float t = Max(t_min.x, Max(t_min.y, t_min.z));
    float tExit  = Min(t_max.x, Min(t_max.y, t_max.z));
    t = Max(t, 0.0f);

    // Exit if no point along the ray intersects the root bounding box of the SVO.
    if (tExit < t) {
        return;
    }
    
    DrawAABB(v0, v1);
    
    // Allocate the stack for recursion through the SVO.
    int lvl = 0;
    SvoStackEntry* stack = ALLOC_ARRAY(TempAllocator, SvoStackEntry, maxDepth + 1);
    SvoStackEntry* current = &stack[lvl];
    ZeroStruct(current);

    float scale = rootScale * 0.5f;
    Vector3 center = (v0 + v1) * 0.5f;
    Vector3 p = rayStart + (rayDirection * t); 

    current->corner = v0;
    if (p.x >= center.x) { current->idx ^= 1; current->corner.x = scale; }   
    if (p.y >= center.y) { current->idx ^= 2; current->corner.y = scale; }   
    if (p.z >= center.z) { current->idx ^= 4; current->corner.z = scale; }   
    
#ifdef _DEBUG
    int indentCount = 0;
    char indents[32]; 
    memset(indents, 0, sizeof(indents));
    LOG_MESSAGE("Push %d:%hhu\n", lvl, current->idx);
    indents[indentCount++] = ' ';
#endif // _DEBUG    

    for (;;) {
        Vector3 upper_corner = current->corner + Vector3{scale, scale, scale};
        
        u8 mask = svo->masksAtLevel[lvl][current->mask_idx];
        if (mask & (1u << current->idx)) {            
            if (lvl < maxDepth) {
                center = (current->corner + upper_corner) * 0.5f;
                scale *= 0.5f;
                
                int previous_lvl = lvl;
                int previous_mask_idx = current->mask_idx;
                Vector3 previous_corner = current->corner;
                
                u8 beforeMask = mask & ((1u << current->idx) - 1u);
                int rank = Popcount8(beforeMask);
                
                current = &stack[++lvl];
                current->mask_idx = svo->firstChild[previous_lvl][previous_mask_idx] + rank;
                current->idx = 0;
                current->corner = previous_corner;
                if (p.x >= center.x) { current->idx ^= 1; current->corner.x += scale; }
                if (p.y >= center.y) { current->idx ^= 2; current->corner.y += scale; }
                if (p.z >= center.z) { current->idx ^= 4; current->corner.z += scale; }

#ifdef _DEBUG
                LOG_MESSAGE("%sPush %d:%hhu\n", indents, lvl, current->idx);
                indents[indentCount++] = ' ';
#endif // _DEBUG
   
                continue;
            } else {
                DrawAABB(current->corner, upper_corner);
            }
        }

        
        float x = (stepDir.x > 0) ? upper_corner.x : current->corner.x;
        float tx = (x - rayStart.x) * invDx;
        
        float y = (stepDir.y > 0) ? upper_corner.y : current->corner.y;
        float ty = (y - rayStart.y) * invDy;
        
        float z = (stepDir.z > 0) ? upper_corner.z : current->corner.z;
        float tz = (z - rayStart.z) * invDz;
        
        s8 stepMask = 0;
        if (tx < ty && tx < tz) {
            t = tx;
            stepMask = 1 * stepDir.x;
        } else if (ty < tz) {
            t = ty;
            stepMask = 2 * stepDir.y;
        } else {
            t = tz;
            stepMask = 4 * stepDir.z;
        }

        // TODO(roger): According to https://www.nvidia.com/docs/IO/88972/nvr-2010-001.pdf
        // POP can be simplified by mirroring the octree.
        // This eliminates the need to check the signs of the ray direction. The stepMask can become unsigned.
        // Then POP can check if (idx & stepMask) == 0 to check if its in bounds of the current level,  otherwise pop() to parent lvl
        
        s8 axisBit = (stepMask >= 0) ? stepMask : -stepMask; 
        s8 isBitSet = (current->idx & axisBit) != 0;
        
        while ((stepMask > 0 && isBitSet) || (stepMask < 0 && !isBitSet)) {
            if (lvl == 0) {
                return;
            }

#ifdef _DEBUG            
            indents[--indentCount] = 0;
            LOG_MESSAGE("%sPop\n", indents);
#endif // _DEBUG
            
            scale *= 2;
            current = &stack[--lvl];
            isBitSet = (current->idx & axisBit) != 0;
        }

        current->idx += stepMask;
        p = rayStart + (rayDirection * t);

#ifdef _DEBUG 
        LOG_MESSAGE("%sAdvance %d:%d\n", indents, lvl, current->idx);
#endif // _DEBUG
        
        if (axisBit & 1) { current->corner.x += scale * stepDir.x; }   
        if (axisBit & 2) { current->corner.y += scale * stepDir.y; }   
        if (axisBit & 4) { current->corner.z += scale * stepDir.z; }  
    }
    
    TempArenaMemoryEnd(tempArena);
}

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

void DrawAABB(Vector3 v0, Vector3 v1, float padding) {
    Vector3 size = v1 - v0; 
    int vertexStart = game.gizmoVertexCount;
    int indexStart = game.gizmoIndexCount;
        
    memcpy(game.gizmoVertices + game.gizmoVertexCount, CubeVertices_XYZ, sizeof(CubeVertices_XYZ));
    game.gizmoVertexCount += countOf(CubeVertices_XYZ);

    for (int i = vertexStart; i < game.gizmoVertexCount; i++) {
        game.gizmoVertices[i].x *= (size.x + padding*2);    
        game.gizmoVertices[i].y *= (size.y + padding*2);    
        game.gizmoVertices[i].z *= (size.z + padding*2);
        game.gizmoVertices[i].x += v0.x - padding;    
        game.gizmoVertices[i].y += v0.y - padding;    
        game.gizmoVertices[i].z += v0.z - padding;    
    }
    
    memcpy(game.gizmoIndices + game.gizmoIndexCount, CubeLineIndices_XYZ, sizeof(CubeLineIndices_XYZ));
    game.gizmoIndexCount += countOf(CubeLineIndices_XYZ);
    
    for (int i = indexStart; i < game.gizmoIndexCount; i++) {
        game.gizmoIndices[i] += vertexStart;
    }
}
