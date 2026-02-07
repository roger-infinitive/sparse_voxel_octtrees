#ifndef _MESH_DATA_H_
#define _MESH_DATA_H_

struct Vertex {
    float x, y;
};

struct Vertex_XYZ {
    float x, y, z;
};

struct Vertex_XYZ_N {
    float x, y, z;
    float nx, ny, nz;
};

struct VertexUV {
    float x, y, z;
    float u, v;
};

struct VertexUV2 {
    float x, y;
    float u, v;
};

struct VertexColor {
    float x, y;
    float r, g, b, a;
};

struct VertexUV2Color {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct VertexUVColor {
    float x, y, z;
    float u, v;
    
    union {
        struct { float r, g, b, a; };
        Vector4 color;  
    };
};

struct VertexMesh {
    u32 vertexCount;
    VertexUV2Color* vertices;
};

struct IndexedMesh {
    u32 vertexCount;
    VertexUV2Color* vertices;
    u32 indexCount;
    u16* indices;
};

Vector2 DefaultUVs[] = {
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f },
    { 0.0f, 0.0f }
};

VertexUV tileVertices[] = {
	{  1.0f,  1.0f, 1.0f, 1.0f, 0.0f },
	{  1.0f,  0.0f, 1.0f, 1.0f, 1.0f },
	{  0.0f,  0.0f, 1.0f, 0.0f, 1.0f },
	{  0.0f,  1.0f, 1.0f, 0.0f, 0.0f }
};

VertexUV2Color VertexUV2Color_Quad[] = {
	{  /* xy */ 1.0f, 1.0f, /* uv */ 1.0f, 0.0f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f },
	{  /* xy */ 1.0f, 0.0f, /* uv */ 1.0f, 1.0f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f },
	{  /* xy */ 0.0f, 0.0f, /* uv */ 0.0f, 1.0f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f },
	{  /* xy */ 0.0f, 1.0f, /* uv */ 0.0f, 0.0f, /* rgba */ 1.0f, 1.0f, 1.0f, 1.0f },
};

u16 DefaultIndices[] = {
	0, 1, 2, 2, 3, 0
};

Vertex_XYZ_N CubeVertices_XYZ_N[] = {
    {0, 0, 0, 0, 0, -1},
    {0, 1, 0, 0, 0, -1},
    {1, 1, 0, 0, 0, -1},
    {1, 0, 0, 0, 0, -1},

    {0, 0, 0, -1, 0, 0},
    {0, 1, 0, -1, 0, 0},
    {0, 1, 1, -1, 0, 0},
    {0, 0, 1, -1, 0, 0},
    
    {0, 0, 0, 0, -1, 0},
    {0, 0, 1, 0, -1, 0},
    {1, 0, 1, 0, -1, 0},
    {1, 0, 0, 0, -1, 0},
    
    {0, 0, 1, 0, 0, 1},
    {0, 1, 1, 0, 0, 1},
    {1, 1, 1, 0, 0, 1},
    {1, 0, 1, 0, 0, 1},

    {1, 0, 0, 1, 0, 0},
    {1, 1, 0, 1, 0, 0},
    {1, 1, 1, 1, 0, 0},
    {1, 0, 1, 1, 0, 0},
    
    {0, 1, 0, 0, 1, 0},
    {0, 1, 1, 0, 1, 0},
    {1, 1, 1, 0, 1, 0},
    {1, 1, 0, 0, 1, 0},
};

u32 CubeTriIndices_XYZ_N[] = {  0,  1,  2,  2,  3,  0,
                                6,  5,  4,  4,  7,  6,
                                8,  11, 10, 10, 9,  8,
                               12, 15, 14, 14, 13, 12,
                               16, 17, 18, 18, 19, 16,
                               20, 21, 22, 22, 23, 20, };

Vertex_XYZ CubeVertices_XYZ[] = {
    {0, 0, 0},
    {0, 1, 0},
    {1, 1, 0},
    {1, 0, 0},
    {0, 0, 1},
    {0, 1, 1},
    {1, 1, 1},
    {1, 0, 1},
};

// for CubeVertices_XYZ
u32 CubeLineIndices_XYZ[] = { 0, 1, 1, 2, 2, 3, 3, 0, 
                             0, 4, 1, 5, 2, 6, 3, 7,
                             4, 5, 5, 6, 6, 7, 7, 4 };
       
// for CubeVertices_XYZ
u32 CubeTriIndices_XYZ[] = { 0, 1, 2, 2, 3, 0,
                            5, 1, 0, 0, 4, 5,
                            4, 7, 6, 6, 5, 4, 
                            2, 6, 7, 7, 3, 2,
                            5, 6, 2, 2, 1, 5,
                            4, 7, 3, 3, 0, 4 };

IndexedMesh GenerateGridMesh(int rows, int cols, void* (*mem_alloc)(size_t), int subdivisionCount = 4) {
    IndexedMesh mesh = {0};
    u32 indexPerRow = (cols*subdivisionCount + 1);
    mesh.vertexCount = (rows+1) * indexPerRow;
    mesh.vertices = (VertexUV2Color*)mem_alloc(sizeof(VertexUV2Color) * mesh.vertexCount);
    u32 currentVertex = 0;
    
    for (int row = 0; row <= rows; row++) {
        for (int col = 0; col < cols; col++) {
            float subSize = 1.0f / (float)subdivisionCount;
            for (int sub = 0; sub < subdivisionCount; sub++) {
                VertexUV2Color* vertex = &mesh.vertices[currentVertex];
                vertex->x = col + (sub * subSize);
                vertex->y = row;
                //TODO(roger): Change UV mapping
                vertex->u = sub * subSize;
                vertex->v = (row % 2);
                vertex->r = 1.0f;
                vertex->g = 1.0f;
                vertex->b = 1.0f;
                vertex->a = 1.0f;
                currentVertex++;
            }
        }
        
        //Add last edge for row
        VertexUV2Color* vertex = &mesh.vertices[currentVertex];
        vertex->x = cols;
        vertex->y = row;
        
        //TODO(roger): Change UV mapping
        vertex->u = (cols % 2);
        vertex->v = (row % 2);
        
        vertex->r = 1.0f;
        vertex->g = 1.0f;
        vertex->b = 1.0f;
        vertex->a = 1.0f;
        
        currentVertex++;
    }
    
    mesh.indexCount = rows * cols * subdivisionCount * 6;
    mesh.indices = (u16*)mem_alloc(sizeof(u16) * mesh.indexCount);
    
    int offset = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            for (int sub = 0; sub < subdivisionCount; sub++) {
                u32 index = (indexPerRow * row) + (col * subdivisionCount) + sub;
                u16 topLeft     = index;
                u16 topRight    = topLeft + 1;
                u16 bottomLeft  = topLeft + indexPerRow;
                u16 bottomRight = bottomLeft + 1;
                
                u16* indices = &mesh.indices[offset];
                indices[0] = topLeft;
                indices[1] = bottomLeft;
                indices[2] = bottomRight;
                indices[3] = bottomRight; 
                indices[4] = topRight; 
                indices[5] = topLeft; 
                offset += 6;
            }
        }
    }
    
    return mesh;
}

//NOTE(roger): This function assumes that the texture used for the mesh uses 9 uniform slices, meaning the sides, corners, and
//center use the same pixel width and height. For example, we can use a 300x300 texture where each slice is 100x100. 
//If the texture is not uniform then the UVs will NOT map correctly. 
VertexMesh GenerateNineSlicedMesh(Vector4 color, Vector2 stretch, MEM_ALLOC_PARAM) {
    VertexMesh mesh = {0};
    mesh.vertices = MEM_ALLOC_ARRAY(54, VertexUV2Color);

    if (stretch.x < 0) stretch.x = 0;
    if (stretch.y < 0) stretch.y = 0;
    for (u32 i = 0; i < 9; i++) {
        u32 stretchWidth = (u32)stretch.x;
        u32 stretchHeight = (u32)stretch.y;
        
        u32 x = i / 3;
        u32 y = i % 3;
        
        float xUv1 = x * 0.33f;
        float xUv2 = (x+1.0f) * 0.33f;
        float yUv1 = y * 0.33f;
        float yUv2 = (y+1.0f) * 0.33f;
        
        float xStretchFactor = 1;
        float xOffset = x;
        if (x == 1) {
            xStretchFactor = stretch.x;
        } else if (x == 2) {
            xOffset = stretch.x + 1;
        }
        
        float yStretchFactor = 1;
        float yOffset = y;
        if (y == 1) {
            yStretchFactor = stretch.y;                        
        } else if (y == 2) {
            yOffset = stretch.y + 1; 
        }
        
        //Left and Right Sides
        if (y == 1 && (x == 0 || x == 2)) {
            float size = 1.0f;
            for (float j = stretch.y; j > 0; j -= size) {
                size = 1.0f;
                if (j < 1.0f) { size = j; }
                float yOffset = stretch.y - j;
                VertexUV2Color* vertexBase = &mesh.vertices[mesh.vertexCount];
                vertexBase[0] = { xOffset,   size + yOffset, xUv1, yUv1, color.x, color.y, color.z, color.w };
                vertexBase[1] = { xOffset,   size + 1 + yOffset, xUv1, yUv2, color.x, color.y, color.z, color.w };
                vertexBase[2] = { xOffset+1, size + 1 + yOffset, xUv2, yUv2, color.x, color.y, color.z, color.w };
                vertexBase[3] = vertexBase[2];
                vertexBase[4] = { xOffset+1, size + yOffset, xUv2, yUv1, color.x, color.y, color.z, color.w };
                vertexBase[5] = vertexBase[0];
                mesh.vertexCount += 6;
            }
        //Top and Bottom Sides
        } else if (x == 1 && (y == 0 || y == 2)) {
            float size = 1.0f;
            for (float j = stretch.x; j > 0; j -= size) {
                size = 1.0f;
                if (j < 1.0f) size = j;
                float xOffset = stretch.x - j;
                VertexUV2Color* vertexBase = &mesh.vertices[mesh.vertexCount];
                vertexBase[0] = { size + xOffset, yOffset,   xUv1, yUv1, color.x, color.y, color.z, 1 };
                vertexBase[1] = { size + xOffset, yOffset+1, xUv1, yUv2, color.x, color.y, color.z, 1 };
                vertexBase[2] = { size + 1 + xOffset, yOffset+1, xUv2, yUv2, color.x, color.y, color.z, 1 };
                vertexBase[3] = vertexBase[2];
                vertexBase[4] = { size + 1 + xOffset, yOffset,   xUv2, yUv1, color.x, color.y, color.z, 1 };
                vertexBase[5] = vertexBase[0]; 
                mesh.vertexCount += 6;
            }
        //Corners and Center
        } else {            
            VertexUV2Color* vertexBase = &mesh.vertices[mesh.vertexCount];
            vertexBase[0] = { (float)xOffset,                (float)yOffset,   xUv1, yUv1, color.x, color.y, color.z, 1 };
            vertexBase[1] = { (float)xOffset,                (float)yOffset+yStretchFactor, xUv1, yUv2, color.x, color.y, color.z, 1 };
            vertexBase[2] = { (float)xOffset+xStretchFactor, (float)yOffset+yStretchFactor, xUv2, yUv2, color.x, color.y, color.z, 1 };
            vertexBase[3] = vertexBase[2];
            vertexBase[4] = { (float)xOffset+xStretchFactor, (float)yOffset,   xUv2, yUv1, color.x, color.y, color.z, 1 };
            vertexBase[5] = vertexBase[0];
            mesh.vertexCount += 6;
        }
    }
    
    return mesh;
}

#endif // _MESH_DATA_H_ 