#include "d3d.cpp"
#include "dxgi.cpp"

//TODO(roger): This should not depend on "renderState" from cultist_render.h.
//For example, DX11_SetView and DX11_SetProjection both use the constant buffers from renderState to update matrices.
//Instead we should set this up inside of the renderer layer and remove them from the cultist_render. This is better, because
//projection and view updates are more of a renderer layer function than a game rendering function.

//TODO(roger): Create a DX11_RenderContext for all of the external variables similar to what we are doing for DX12.

void* stencilModes[StencilMode_Count];
IDXGISwapChain1* swapChain;
// This will be 0 if the swap chain was created without DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT.
// Or if we fail to get the swapChain2. Either way, check if its 0 before usage.
HANDLE frameLatencyWaitableObject;
ID3D11Device* device;
ID3D11DeviceContext* imContext;
ID3D11RenderTargetView* swapchainRTV;
ID3D11DepthStencilView* depthStencilView;
ID3D11RasterizerState* rasterizerSolid;
ID3D11RasterizerState* rasterizerWireframe;
ID3D11RenderTargetView* activeRenderTarget;
D3D11_VIEWPORT viewport = {};

//Render info
Matrix4 projectionMatrix; // typically updated once at beginning of program

void DX11_CreateSwapchainRTV() {
    ID3D11Texture2D* buffer = 0;
    HRESULT result = swapChain->GetBuffer(0, IID_PPV_ARGS(&buffer));
    ASSERT_ERROR(result == 0, "(DX11) Failed to get the back buffer from swap chain.\n");

    result = device->CreateRenderTargetView(buffer, 0, &swapchainRTV);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create render target view.\n");

    buffer->Release();
}

void DX11_CreateDepthStencilView(int width, int height) {
    D3D11_TEXTURE2D_DESC depthStencilDesc = {0};
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    ID3D11Texture2D* depthStencilBuffer;
    HRESULT result = device->CreateTexture2D(&depthStencilDesc, 0, &depthStencilBuffer);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create depth stencil buffer.\n");

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {0};
    depthStencilViewDesc.Format = depthStencilDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    result = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create depth stencil view.\n");

    // Release the depth stencil buffer after creating the view
    depthStencilBuffer->Release();
}

void DX11_SetViewport(int width, int height) {
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
}

void DX11_InitializeStencilModes() {
	D3D11_DEPTH_STENCIL_DESC noneStencilDesc = {0};
	noneStencilDesc.DepthEnable = TRUE;
    noneStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    noneStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	HRESULT result = device->CreateDepthStencilState(&noneStencilDesc, (ID3D11DepthStencilState**)&stencilModes[StencilMode_None]);
	ASSERT_ERROR(result == 0, "(DX11) Failed to create stencil state (none).\n");

    D3D11_DEPTH_STENCIL_DESC writeDesc = {};
    writeDesc.DepthEnable = TRUE;
    writeDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    writeDesc.DepthFunc = D3D11_COMPARISON_LESS;
    writeDesc.StencilEnable = TRUE;
    writeDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    writeDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    writeDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    writeDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    writeDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    writeDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    writeDesc.BackFace = writeDesc.FrontFace;
	result = device->CreateDepthStencilState(&writeDesc, (ID3D11DepthStencilState**)&stencilModes[StencilMode_Write]);
	ASSERT_ERROR(result == 0, "(DX11) Failed to create stencil state (write mask).\n");

    D3D11_DEPTH_STENCIL_DESC readDesc = {0};
    readDesc.StencilEnable = true;
    readDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	readDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	readDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	readDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	readDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	readDesc.BackFace = readDesc.FrontFace;
	result = device->CreateDepthStencilState(&readDesc, (ID3D11DepthStencilState**)&stencilModes[StencilMode_Read]);
	ASSERT_ERROR(result == 0, "(DX11) Failed to create stencil state (read mask).\n");
}


void DX11_CreateSRV(Texture* texture, int mipLevels, int mostDetailedMip, ID3D11ShaderResourceView** srv) {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { };
    switch(texture->format) {
        case TextureFormat_BGRA8_UNORM: srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        case TextureFormat_RGBA8_UNORM: srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case TextureFormat_A8_UNORM:    srvDesc.Format = DXGI_FORMAT_A8_UNORM;       break;
        case TextureFormat_BC7_UNORM:   srvDesc.Format = DXGI_FORMAT_BC7_UNORM;      break;
        default: NOT_IMPLEMENTED(); break;
    }
    
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
    srvDesc.Texture2D.MipLevels = mipLevels;
    
    HRESULT result = device->CreateShaderResourceView(texture->dx11.resource, &srvDesc, srv);
    ASSERT_ERROR(result == 0, "Failed to create texture view.\n");
}

bool DX11_LoadTexture(u8* data, Texture* texture) {
    ASSERT_DEBUG(texture->mipLevels != 0, "MIP Levels must be at least 1!");

    D3D11_TEXTURE2D_DESC textureDesc = { };
    textureDesc.Width = texture->width;
    textureDesc.Height = texture->height;
    textureDesc.MipLevels = texture->mipLevels;
    textureDesc.ArraySize = 1;

    switch(texture->format) {
        case TextureFormat_BGRA8_UNORM: textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        case TextureFormat_RGBA8_UNORM: textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case TextureFormat_A8_UNORM:    textureDesc.Format = DXGI_FORMAT_A8_UNORM;       break;
        case TextureFormat_BC7_UNORM:   textureDesc.Format = DXGI_FORMAT_BC7_UNORM;      break;
        default: NOT_IMPLEMENTED(); break;
    }

    textureDesc.SampleDesc.Count = 1;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA* initData = ALLOC_ARRAY(HeapAllocator, D3D11_SUBRESOURCE_DATA, texture->mipLevels);
    memset(initData, 0, texture->mipLevels * sizeof(D3D11_SUBRESOURCE_DATA));

    int width = texture->width;
    int height = texture->height;
    size_t offset = 0;
    for (int i = 0; i < texture->mipLevels; i++) {
        initData[i].pSysMem = data + offset;

        UINT rowPitch = 0;
        size_t mipSize  = 0;
        switch (texture->format) {
            case TextureFormat_BGRA8_UNORM:
            case TextureFormat_RGBA8_UNORM: {
                rowPitch = width * 4;
                mipSize = rowPitch * height;
            } break;

            case TextureFormat_A8_UNORM: {
                rowPitch = width;
                mipSize = rowPitch * height;
            } break;

            case TextureFormat_BC7_UNORM: {
                rowPitch = ((width + 3) / 4) * 16;
                mipSize = ((height + 3) / 4) * rowPitch;
            } break;

            default: NOT_IMPLEMENTED(); break;
        }

        initData[i].SysMemPitch = rowPitch;
        initData[i].SysMemSlicePitch = 0;

        offset += mipSize;
        width = width / 2;
        height = height / 2;
    }

    HRESULT result = device->CreateTexture2D(&textureDesc, initData, &texture->dx11.resource);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create texture handle.\n");

    HeapFree(initData);

    DX11_CreateSRV(texture, texture->mipLevels, 0, &texture->dx11.srv);
  
    texture->isRenderTarget = false;
    return result == 0;
}

void DX11_UpdateTexture(Texture* texture, u8* data) {
    int rowPitch = 0;
    int depthPitch = 0;

	switch (texture->format) {
        case TextureFormat_BGRA8_UNORM:
		case TextureFormat_RGBA8_UNORM: rowPitch = texture->width * 4; break;
		case TextureFormat_A8_UNORM:    rowPitch = texture->width; break;
        case TextureFormat_BC7_UNORM: {
            rowPitch = ((texture->width + 3) / 4) * 16;
            depthPitch = rowPitch / texture->height;
        } break;
	    default: NOT_IMPLEMENTED(); break;
	}

    // We are updating the entire texture.
    D3D11_BOX dest;
    dest.left = 0;
    dest.right = texture->width;
    dest.top = 0;
    dest.bottom = texture->height;
    dest.front = 0;
    dest.back = 1;

    // Update the texture subresource
    imContext->UpdateSubresource(texture->dx11.resource, 0, &dest, data, rowPitch, depthPitch);
}

void* DX11_CreateTextureSampler() {
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    ID3D11SamplerState* state = 0;
    HRESULT result = device->CreateSamplerState(&samplerDesc, &state);
    ASSERT_ERROR(result == 0, "Failed to create sample state.");

    return state;
}

void DX11_CreateRenderTexture(Texture* texture, u32 width, u32 height) {
    D3D11_TEXTURE2D_DESC textureDesc = {0};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    HRESULT result = device->CreateTexture2D(&textureDesc, 0, &texture->dx11.resource);
    ASSERT_ERROR(result == 0, "Failed to Create Texture.");

    result = device->CreateRenderTargetView(texture->dx11.resource, 0, &texture->dx11.rtv);
    ASSERT_ERROR(result == 0, "Failed to Render Target for Texture.");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {0};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    result = device->CreateShaderResourceView(texture->dx11.resource, &srvDesc, &texture->dx11.srv);
    ASSERT_ERROR(result == 0, "Failed to Create Shader Resource View");

    texture->isRenderTarget = true;
}

void DX11_ResizeRenderTexture(Texture* renderTexture, u32 width, u32 height) {
    renderTexture->dx11.rtv->Release();
    renderTexture->dx11.srv->Release();
    renderTexture->dx11.resource->Release();
    DX11_CreateRenderTexture(renderTexture, width, height);
}

void DX11_ReleaseTexture(Texture* texture) {
    texture->dx11.resource->Release();
    texture->dx11.resource = 0;
    texture->dx11.srv->Release();
    texture->dx11.srv = 0;

    if (texture->dx11.rtv) {
        texture->dx11.rtv->Release();
        texture->dx11.rtv = 0;
    }
}

void DX11_ReleaseBuffer(GpuBuffer* buffer) {
    buffer->dx11.buffer->Release();
    memset(buffer, 0, sizeof(GpuBuffer));
}

ShaderProgram DX11_LoadShader(const char* path, VertexLayoutType vertexLayoutType) {
    ShaderProgram program = D3D_LoadShaderText(path, path);
    program.backend = ShaderBackend_D3D;

    ID3DBlob* vertexBytecode = (ID3DBlob*)program.d3d.vsBytecode;
    ID3DBlob* pixelBytecode  = (ID3DBlob*)program.d3d.psBytecode;

    HRESULT result = device->CreateVertexShader(vertexBytecode->GetBufferPointer(), vertexBytecode->GetBufferSize(), 0, (ID3D11VertexShader**)&program.d3d.vertexShader);
    ASSERT_ERROR(result == 0, "Failed to create vertex shader.");

    result = device->CreatePixelShader(pixelBytecode->GetBufferPointer(), pixelBytecode->GetBufferSize(), 0, (ID3D11PixelShader**)&program.d3d.pixelShader);
    ASSERT_ERROR(result == 0, " Failed to create pixel shader.");

    // Create input layout if not already created.
    // NOTE(roger): Unfortunately DirectX 11 requires vertex shader bytecode inorder to validate input layout creation...
    if (vertexLayoutType != VertexLayout_None && !vertexLayouts[vertexLayoutType].created) {

        u32 count = 0;
        D3D11_INPUT_ELEMENT_DESC layout[16];

        switch (vertexLayoutType) {
            case VertexLayout_XY: {
                layout[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
            } break;

            case VertexLayout_XY_UV: {
                layout[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                layout[count++] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 };
            } break;

            case VertexLayout_XY_RGBA: {
                layout[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                layout[count++] = { "TINT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 };
            } break;

            case VertexLayout_XY_UV_RGBA: {
                layout[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 };
                layout[count++] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 };
                layout[count++] = { "TINT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 };
            } break;

            case VertexLayout_XYZ: {
                layout[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
            } break;
            
            case VertexLayout_XYZ_NORMAL: {
                layout[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,  D3D11_INPUT_PER_VERTEX_DATA, 0 };
                layout[count++] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  12, D3D11_INPUT_PER_VERTEX_DATA, 0 };
            } break;
            
            case VertexLayout_XYZ_UV_RGBA: {
                layout[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                layout[count++] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 };
                layout[count++] = { "TINT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 };
            } break;
            
            default: NOT_IMPLEMENTED();
        }

        result = device->CreateInputLayout(layout, count,
            vertexBytecode->GetBufferPointer(), vertexBytecode->GetBufferSize(),
            &vertexLayouts[vertexLayoutType].dx11.layout);

        ASSERT_ERROR(result == 0, "Failed to create input layout %d for vertex layout.", vertexLayoutType);
        vertexLayouts[vertexLayoutType].created = true;

        vertexBytecode->Release();
        pixelBytecode->Release();

        program.d3d.vsBytecode = 0;
        program.d3d.psBytecode = 0;
    }

    return program;
}

void DX11_CreateConstantBuffer(u32 paramIndex, ConstantBuffer* constantBuffer, size_t bufferSize) {
    ASSERT_ERROR(bufferSize % 16 == 0, "Constant buffer has to be aligned by 16 bytes. Current size: %zd", bufferSize);

    D3D11_BUFFER_DESC bufferDesc = { };
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.ByteWidth = bufferSize;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;

    HRESULT result = device->CreateBuffer(&bufferDesc, 0, &constantBuffer->dx11.buffer);
    ASSERT_ERROR(result == 0, "Failed to create constant buffer: %d", result);

    constantBuffer->paramIndex = paramIndex;
}

void DX11_UpdateConstantBuffer(ConstantBuffer* constantBuffer, void* data, size_t size) {
    imContext->UpdateSubresource(constantBuffer->dx11.buffer, 0, 0, data, 0, 0);
}

void DX11_BindConstantBuffers(u32 startSlot, ConstantBuffer** constantBuffers, u32 count) {
    const u32 MAX_CONSTANT_BUFFERS = 8;
    ASSERT_ERROR(count <= MAX_CONSTANT_BUFFERS, "Exceed max constant buffers.");

    ID3D11Buffer* buffers[MAX_CONSTANT_BUFFERS];
    for (u32 i = 0; i < count; i++) {
        buffers[i] = constantBuffers[i]->dx11.buffer;
    }

    imContext->VSSetConstantBuffers(startSlot, count, buffers);
    imContext->PSSetConstantBuffers(startSlot, count, buffers);
}

#define MAX_SHADER_TEXTURES 8

void DX11_UnbindShaderTextures(u32 count) {
    ASSERT_ERROR(count <= MAX_SHADER_TEXTURES, "Exceeded max shader textures.");

    ID3D11ShaderResourceView* srvs[MAX_SHADER_TEXTURES];
    for (u32 i = 0; i < count; i++) {
        srvs[i] = 0;
    }

    imContext->PSSetShaderResources(0, count, srvs);
}

void DX11_BindShaderTextures(u32 startSlot, Texture** resources, u32 count) {
    ASSERT_ERROR(count <= MAX_SHADER_TEXTURES, "Exceeded max shader textures.");

    ID3D11ShaderResourceView* srvs[MAX_SHADER_TEXTURES];
    for (u32 i = 0; i < count; i++) {
        srvs[i] = resources[i]->dx11.srv;
    }

    imContext->PSSetShaderResources(startSlot, count, srvs);
}

void DX11_BindVertexBuffers(GpuBuffer** buffers, u32 bufferCount) {
    for (u32 i = 0; i < bufferCount; i++) {
        u32 offset = 0;
        imContext->IASetVertexBuffers(i, 1, &buffers[i]->dx11.buffer, &buffers[i]->stride, &offset);
    }
}

void DX11_BindIndexBuffer(GpuBuffer* buffer) {
    switch (buffer->indexFormat) {
        case IndexFormat_None: { ASSERT_ERROR(false, "This is not a valid Index Buffer!"); } break;

        case IndexFormat_U16: {
            imContext->IASetIndexBuffer(buffer->dx11.buffer, DXGI_FORMAT_R16_UINT, 0);
        } break;

        case IndexFormat_U32: {
            imContext->IASetIndexBuffer(buffer->dx11.buffer, DXGI_FORMAT_R32_UINT, 0);
        } break;
    }
}

void DX11_SetTextureSamplers(u32 startSlot, void** samplers, u32 count) {
    imContext->PSSetSamplers(startSlot, count, (ID3D11SamplerState**)samplers);
}

void DX11_BeginDrawing() {
    imContext->OMSetRenderTargets(1, &swapchainRTV, depthStencilView);
    imContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    activeRenderTarget = swapchainRTV;
}

void DX11_EndDrawing() {
    //TODO(roger): Does nothing right now. Throw error? Unset state?
}

void DX11_DrawVertices(u32 vertexCount, u32 offset) {
    imContext->Draw(vertexCount, offset);
}

void DX11_DrawIndexedVertices(u32 indexCount, u32 startIndex, int baseVertex) {
    imContext->DrawIndexed(indexCount, startIndex, baseVertex);
}

void DX11_BeginTextureMode(Texture* renderTexture) {
    activeRenderTarget = renderTexture->dx11.rtv;
    imContext->OMSetRenderTargets(1, &activeRenderTarget, depthStencilView);
    imContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void DX11_EndTextureMode() {
    activeRenderTarget = 0;
}

void DX11_SetScissor(Vector2 point, Vector2 size) {
    D3D11_RECT scissorRect;
    scissorRect.left = point.x;
    scissorRect.top = point.y;
    scissorRect.right = point.x + size.x;
    scissorRect.bottom = point.y + size.y;
    imContext->RSSetScissorRects(1, &scissorRect);
}

void DX11_ClearBackground(Vector4 color) {
    imContext->ClearRenderTargetView((ID3D11RenderTargetView*)activeRenderTarget, &color.x);
}

void DX11_GraphicsPresent() {
	//vsync (true) uses 1
	swapChain->Present(0, 0);
}

void DX11_MapBuffer(GpuBuffer* buffer, bool writeDiscard = true) {
    D3D11_MAPPED_SUBRESOURCE mappedSpriteBuffer;
    HRESULT result = imContext->Map(buffer->dx11.buffer, 0, writeDiscard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedSpriteBuffer);
    ASSERT_ERROR(result == 0, "Failed to map buffer for write.");
    buffer->count = 0;
    buffer->mapped = mappedSpriteBuffer.pData;
}

void DX11_UnmapBuffer(GpuBuffer* buffer) {
    imContext->Unmap(buffer->dx11.buffer, 0);
    buffer->mapped = 0;
}

void DX11_CreateGraphicsBuffer(GpuBuffer* buffer, void* initialData) {
    D3D11_BUFFER_DESC desc = {};

    // Set buffer type
    switch (buffer->type) {
        case VertexBuffer: {
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        } break;
        case IndexBuffer: {
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        } break;
    }

    // Set buffer usage
    switch (buffer->usage) {
        case StaticDraw: {
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.CPUAccessFlags = 0;
        } break;

        case DynamicDraw: {
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        } break;
    }

    // Set up initial data
    D3D11_SUBRESOURCE_DATA* initData = 0;
    D3D11_SUBRESOURCE_DATA subresourceData = {};
    if (initialData != 0) {
        subresourceData.pSysMem = initialData;
        initData = &subresourceData;
    }

    desc.ByteWidth = buffer->stride * buffer->capacity;
    desc.MiscFlags = 0;

    HRESULT result = device->CreateBuffer(&desc, initData, &buffer->dx11.buffer);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create buffer.\n");
}

D3D11_BLEND DX11_Blend(BlendType type) {
    switch (type) {
        case BlendType_Zero:          { return D3D11_BLEND_ZERO;          } break;
        case BlendType_One:           { return D3D11_BLEND_ONE;           } break;
        case BlendType_Src_Alpha:     { return D3D11_BLEND_SRC_ALPHA;     } break;
        case BlendType_Inv_Src_Alpha: { return D3D11_BLEND_INV_SRC_ALPHA; } break;

        default: NOT_IMPLEMENTED(); return D3D11_BLEND_ZERO;
    }
}

D3D11_BLEND_OP DX11_BlendOp(BlendOp op) {
    switch (op) {
        case BlendOp_Add:           { return D3D11_BLEND_OP_ADD;          } break;
        case BlendOp_Subtract:      { return D3D11_BLEND_OP_SUBTRACT;     } break;
        case BlendOp_Rev_Subtract:  { return D3D11_BLEND_OP_REV_SUBTRACT; } break;
        case BlendOp_Min:           { return D3D11_BLEND_OP_MIN;          } break;
        case BlendOp_Max:           { return D3D11_BLEND_OP_MAX;          } break;

        default: NOT_IMPLEMENTED(); return D3D11_BLEND_OP_ADD;
    }
}

void DX11_CreatePipelineState(PipelineState* state) {
    // Create Blend State
    D3D11_BLEND_DESC blendDesc = {0};
    blendDesc.RenderTarget[0].BlendEnable    = state->blendDesc.enableBlend;
    blendDesc.RenderTarget[0].SrcBlend       = DX11_Blend(state->blendDesc.srcBlend);
    blendDesc.RenderTarget[0].DestBlend      = DX11_Blend(state->blendDesc.destBlend);
    blendDesc.RenderTarget[0].BlendOp        = DX11_BlendOp(state->blendDesc.blendOp);
    blendDesc.RenderTarget[0].SrcBlendAlpha  = DX11_Blend(state->blendDesc.srcBlendAlpha);
    blendDesc.RenderTarget[0].DestBlendAlpha = DX11_Blend(state->blendDesc.destBlendAlpha);
    blendDesc.RenderTarget[0].BlendOpAlpha   = DX11_BlendOp(state->blendDesc.blendOpAlpha);
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT result = device->CreateBlendState(&blendDesc, &state->dx11.blendState);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create blend state");
}

void DX11_SetPipelineState(PipelineState* state) {
    if (state->vertexLayout == VertexLayout_None) {
        imContext->IASetInputLayout(0);
    } else {
        imContext->IASetInputLayout(vertexLayouts[state->vertexLayout].dx11.layout);
    }

    D3D11_PRIMITIVE_TOPOLOGY d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    switch (state->topology) {
        case PrimitiveTopology_TriangleList: { d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; } break;
        case PrimitiveTopology_LineList:     { d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;     } break;
        case PrimitiveTopology_LineStrip:    { d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;    } break;
        default: { NOT_IMPLEMENTED(); } break;
    }
    imContext->IASetPrimitiveTopology(d3dTopology);

    ID3D11RasterizerState* rasterState = 0;
    switch(state->rasterizer) {
        case Rasterizer_Default:   { rasterState = rasterizerSolid;     } break;
        case Rasterizer_Wireframe: { rasterState = rasterizerWireframe; } break;
        default: { NOT_IMPLEMENTED(); } break;
    }
    imContext->RSSetState(rasterState);

    imContext->VSSetShader((ID3D11VertexShader*)state->shader->d3d.vertexShader, 0, 0);
    imContext->PSSetShader((ID3D11PixelShader*)state->shader->d3d.pixelShader, 0, 0);

	imContext->OMSetBlendState(state->dx11.blendState, 0, 0xFFFFFFFF);

	UINT stencilRef = 0;
    switch (state->stencilMode) {
        case StencilMode_Write:
        case StencilMode_Read: { stencilRef = 1; } break;
    }
    imContext->OMSetDepthStencilState((ID3D11DepthStencilState*)stencilModes[state->stencilMode], stencilRef);
}

void DX11_NewFrame() {
    imContext->RSSetViewports(1, &viewport);
    DX11_SetScissor({0, 0}, {(float)viewport.Width, (float)viewport.Height});
}

// TODO(roger): store current swapChainCreationFlags for ResizeBuffers.
void DX11_ResizeGraphics(u32 width, u32 height, Texture* screenTextures, int screenTextureCount) {
    //Resize swapchain buffer and recreate the render target view.
    imContext->OMSetRenderTargets(0, 0, 0);
    swapchainRTV->Release();
    depthStencilView->Release();

    UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    // TODO(roger): Handle vsync. Resizing buffers should be in our renderer, and we should track swap chain properties.
    // if (!vSync) {
        swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    // }

    // NOTE(roger): set bufferCount: 0 and format: DXGI_FORMAT_UNKNOWN to preserve original values.
    // We must pass in the same swapChainFlags if we wish to preserve them.
    HRESULT result = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, swapChainFlags);
    if (FAILED(result)) {
        char errorMsg[256];
        sprintf(errorMsg, "ResizeBuffers failed with HRESULT: 0x%08X\n", result);
        ASSERT_ERROR(false, errorMsg);
    }

    DX11_CreateSwapchainRTV();
    DX11_CreateDepthStencilView(width, height);
    DX11_SetViewport(width, height);

    for (u32 i = 0; i < screenTextureCount; i++) {
        DX11_ResizeRenderTexture(&screenTextures[i], width, height);
    }
}

void DX11_InitializeRenderer(HWND window) {
    LoadShader = DX11_LoadShader;

    CreateConstantBuffer = DX11_CreateConstantBuffer;
    BindConstantBuffers = DX11_BindConstantBuffers;
    UpdateConstantBuffer = DX11_UpdateConstantBuffer;

    CreateGraphicsBuffer = DX11_CreateGraphicsBuffer;
    ReleaseBuffer = DX11_ReleaseBuffer;
    MapBuffer = DX11_MapBuffer;
    UnmapBuffer = DX11_UnmapBuffer;
    BindVertexBuffers = DX11_BindVertexBuffers;
    BindIndexBuffer = DX11_BindIndexBuffer;
    UnbindShaderTextures = DX11_UnbindShaderTextures;
    BindShaderTextures = DX11_BindShaderTextures;

    BeginDrawing = DX11_BeginDrawing;
    EndDrawing = DX11_EndDrawing;
    BeginTextureMode = DX11_BeginTextureMode;
    EndTextureMode = DX11_EndTextureMode;
    DrawVertices = DX11_DrawVertices;
    DrawIndexedVertices = DX11_DrawIndexedVertices;

    LoadTexture = DX11_LoadTexture;
    UpdateTexture = DX11_UpdateTexture;
    CreateRenderTexture = DX11_CreateRenderTexture;
    ReleaseTexture = DX11_ReleaseTexture;
    CreateTextureSampler = DX11_CreateTextureSampler;
    SetTextureSamplers = DX11_SetTextureSamplers;

    SetScissor = DX11_SetScissor;
    CreatePipelineState = DX11_CreatePipelineState;
    SetPipelineState = DX11_SetPipelineState;

    ClearBackground = DX11_ClearBackground;
    ResizeGraphics = DX11_ResizeGraphics;
    GraphicsPresent = DX11_GraphicsPresent;
    NewFrame = DX11_NewFrame;

    // Compute the exact client dimensions for the render targets
    RECT clientRect;
    GetClientRect(window, &clientRect);
    u32 width = clientRect.right - clientRect.left;
    u32 height = clientRect.bottom - clientRect.top;

    u32 createDeviceFlags = 0;
#if _DEBUG
	createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

    // TODO(roger): Do we need 11_1?
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL featureLevel;

    IDXGIAdapter* adapter = DXGI_PickBestAdapter();

    D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, createDeviceFlags,
        featureLevels, countOf(featureLevels), D3D11_SDK_VERSION,
        &device, &featureLevel, &imContext);

    adapter->Release();

    // TODO(roger): Add fallback to older version 11_0 if 11_1 is unsupported?
    ASSERT_ERROR(featureLevel >= D3D_FEATURE_LEVEL_11_0, "DirectX 11 not supported.");

    IDXGIFactory4* dxgiFactory = 0;
    HRESULT result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&dxgiFactory);
    ASSERT_ERROR(result == 0, "(DX12) Failed to create dxgi factory.");

    // Create swap chain that guarentees Flip mode and things like triple buffering.
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = width;
    scDesc.Height = height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SampleDesc.Count = 1;
    // TODO(roger): expose as a setting in a Video menu (double buffer, triple buffer, i.e.)?
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Scaling = DXGI_SCALING_NONE;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    // if (!vSync) {
        scDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    // }

    result = dxgiFactory->CreateSwapChainForHwnd(device, window, &scDesc, 0, 0, &swapChain);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create swap chain.");
    dxgiFactory->Release();

    IDXGISwapChain2* swapChain2;
    swapChain->QueryInterface(__uuidof(IDXGISwapChain2), (void**)&swapChain2);
    if (swapChain2 != 0) {
        swapChain2->SetMaximumFrameLatency(1);
        frameLatencyWaitableObject = swapChain2->GetFrameLatencyWaitableObject();
    }

    DisableAltEnter(window);

    // Check if BC7 format is supported.
    u32 formatSupport = 0;
    result = device->CheckFormatSupport(DXGI_FORMAT_BC7_UNORM, &formatSupport);
    ASSERT_ERROR(result == 0 && (formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D), "(DX11) BC7 is not supported.");

	DX11_CreateSwapchainRTV();
	DX11_CreateDepthStencilView(width, height);
    DX11_SetViewport(width, height);

    //Create solid rasterizer
    D3D11_RASTERIZER_DESC rasterDesc = { };
    rasterDesc.AntialiasedLineEnable = false;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = true;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    result = device->CreateRasterizerState(&rasterDesc, &rasterizerSolid);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create rasterizer state (solid).\n");

    //Create wireframe rasterizer
    rasterDesc.ScissorEnable = true;
    rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    result = device->CreateRasterizerState(&rasterDesc, &rasterizerWireframe);
    ASSERT_ERROR(result == 0, "(DX11) Failed to create rasterizer state (wireframe).\n");

    DX11_InitializeStencilModes();
}