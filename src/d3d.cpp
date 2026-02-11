#include <d3dcompiler.h>
#include "temp_allocator.h"
#include "file_io.h"

// TODO(roger): Move to renderer_directx11
struct ComputeShader {
    union {
        struct {
            ID3DBlob* blob;
            ID3D11ComputeShader* shader;
        } dx11;
    };
};

void PrintShaderErrors(ID3DBlob* errors) {
    if (!errors) { 
        return;
    }

    const char* msg = (const char*)errors->GetBufferPointer();
    size_t len = errors->GetBufferSize();

    fwrite(msg, 1, len, stderr);
    fputc('\n', stderr);

    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

bool D3D_LoadComputeShaderFromFile(const char* computeShaderFile, ID3DBlob** blob) {
    ComputeShader shader = {};
    
    TempArenaMemory tempMemory = TempArenaMemoryBegin(&tempAllocator);
        MemoryBuffer mb;
        ReadEntireFileAndNullTerminate(computeShaderFile, &mb, TempAllocator);
    
        ID3DBlob* errorMsgs = 0;
        HRESULT result = D3DCompile(
            mb.buffer,                             // pointer to source code
            mb.size,                               // size of source code
            0,                                     // optional: source file name
            0,                                     // optional: macros
            0,                                     // optional: include handler
            "CS",                                  // entry point
            "cs_5_0",                              // target profile
            0,                                     // flags1
            0,                                     // flags2
            blob,                                  // compiled bytecode
            &errorMsgs                             // errors (if any)
        );
        
        if (result != 0) {
            #ifdef _DEBUG
                PrintShaderErrors(errorMsgs);
            #endif
            ASSERT_DEBUG(result == 0, "Failed to compile compute shader: %s", computeShaderFile);
            return false;
        }
        LOG_MESSAGE("Compiled compute shader: %s\n", computeShaderFile);
    TempArenaMemoryEnd(tempMemory);
    
    return true;
}

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
            (ID3DBlob**)&program.d3d.vsBytecode,   // compiled bytecode
            &errorMsgs                             // errors (if any)
        );
        
        #ifdef _DEBUG
        PrintShaderErrors(errorMsgs);
        #endif
        ASSERT_ERROR(result == 0, "Failed to compile vertex shader: %s\n", vertexShaderPath);
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
        
        #ifdef _DEBUG
        PrintShaderErrors(errorMsgs);
        #endif
        ASSERT_ERROR(result == 0, "Failed to compile pixel shader: %s\n", pixelShaderPath);
        LOG_MESSAGE("Compiled pixel shader: %s\n", pixelShaderPath);
    TempArenaMemoryEnd(tempMemory);
    
    return program;
}