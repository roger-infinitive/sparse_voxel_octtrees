#define GIZMO_VERTEX_COUNT  640000
#define GIZMO_INDEX_COUNT  1280000

struct Game {
    ConstantBuffer gameConstantBuffer;
    ConstantBuffer frameConstantBuffer;
    
    ShaderProgram simpleShader;
    ShaderProgram simpleLightShader;
    ComputeShader testComputeShader;
    
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
    
    SvoImport svo;
    Camera camera;
    bool hide_model;
};

Game game;

void* ArenaAlloc(size_t size) {
    return PushMemory(&game.memArena, size);
}

Allocator ArenaAllocator {
    ArenaAlloc, 0, 0
};

void RaycastSvo(SvoImport* svo, float rootScale, Vector3 rayStart, Vector3 rayDirection, int maxDepth);
void PackSvoMesh(SvoImport* svo, int lvl);
void DrawLine(Vector3 v0, Vector3 v1);
void DrawAABB(Vector3 v0, Vector3 v1, float padding = 0.0001f);