#include <dxgi1_6.h>

// TODO(roger): This is nice for a quick default, but we want users to be able to choose their adapter. 
IDXGIAdapter1* DXGI_PickBestAdapter() {
    IDXGIFactory6* factory = 0;
    HRESULT hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory6), (void**)&factory);
    if (FAILED(hr)) {
        ASSERT_ERROR(false, "Failed to create DXGI Factory.");
        return 0;
    }

    IDXGIAdapter1* bestAdapter = 0;
    SIZE_T maxVideoMemory = 0;

    // Enumerate adapters
    for (UINT i = 0; ; ++i) {
        IDXGIAdapter1* adapter = 0;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // TODO(roger): Perhaps we allow software adapters but prioritize dedicated
        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter->Release();
            continue;
        }

        // Print adapter info
#if _DEBUG
        wprintf(L"Adapter: %s, VRAM: %llu MB\n", desc.Description, desc.DedicatedVideoMemory / (1024 * 1024));
#endif

        // Select adapter with the most VRAM
        if (desc.DedicatedVideoMemory > maxVideoMemory) {
            if (bestAdapter) { 
                // Release previous best adapter
                bestAdapter->Release(); 
            }
            bestAdapter = adapter;
            maxVideoMemory = desc.DedicatedVideoMemory;
        } else {
            adapter->Release();
        }
    }

    if (factory) { factory->Release(); } 
    return bestAdapter;
}

// TODO(roger): Look into adding "exclusive fullscreen" mode. Right now we use F11 for borderless fullscreen.
void DisableAltEnter(HWND window) {
    IDXGIFactory* dxgiFactory = 0;
    HRESULT result = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    if (result == S_OK) {
        dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
        dxgiFactory->Release();   
    }
}
