#include <d3dcompiler.h>
#include "temp_allocator.h"
#include "file_io.h"

ShaderProgram D3D_LoadShaderText(const char* vertexShaderPath, const char* pixelShaderPath) {
    ShaderProgram program = {};
    
    TempArenaMemory tempMemory = TempArenaMemoryBegin(&tempAllocator);
        // Compile vertex shader
        MemoryBuffer vertexShader;
        ReadEntireFileAndNullTerminate(vertexShaderPath, &vertexShader, TempAllocator);
    
        ID3DBlob* errorMsgs = 0;
        HRESULT result = D3DCompile(
            vertexShader.buffer,                   // pointer to source code
            vertexShader.size,                     // size of source code
            0,                                     // optional: source file name
            0,                                     // optional: macros
            0,                                     // optional: include handler
            "VS",                                  // entry point
            "vs_5_0",                              // target profile
            0,                                     // flags1
            0,                                     // flags2
            (ID3DBlob**)&program.d3d.vsBytecode,        // compiled bytecode
            &errorMsgs                             // errors (if any)
        );
        
        ASSERT_ERROR(result == 0, "Failed to compile vertex shader.");
        LOG_MESSAGE("Compiled vertex shader: %s\n", vertexShaderPath);
    
        // Compile pixel shader
        MemoryBuffer pixelShader;
        ReadEntireFileAndNullTerminate(pixelShaderPath, &pixelShader, TempAllocator);
    
        result = D3DCompile(
            pixelShader.buffer,             // pointer to source code
            pixelShader.size,               // size of source code
            0,                              // optional: source file name
            0,                              // optional: macros
            0,                              // optional: include handler
            "PS",                           // entry point
            "ps_5_0",                       // target profile
            0,                              // flags1
            0,                              // flags2
            (ID3DBlob**)&program.d3d.psBytecode, // compiled bytecode
            &errorMsgs                      // errors (if any)
        );
        
        ASSERT_ERROR(result == 0, "Failed to compile pixel shader.");
        LOG_MESSAGE("Compiled pixel shader: %s\n", pixelShaderPath);
    TempArenaMemoryEnd(tempMemory);
    
    return program;
}