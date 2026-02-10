#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include "platform_windows.h"

#include "utility.h"

#define RENDER_D3D11
#include "renderer.h"
#include "renderer_directx11.cpp"
#include "renderer.cpp"

Vector2 GetClientSize();

#include "game.cpp"
#include "input_windows.cpp"

#define IDI_MYAPP_ICON 101

bool rendererInitialized;
LPCSTR windowName = "SVO"; 
LPCSTR windowClassName = "SVOWindow"; 
HWND window;
WINDOWPLACEMENT previousWindowPlacement = { sizeof(previousWindowPlacement) };

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    #ifndef PACKAGE_FILES
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }
    #endif

    // Register and create window
    {
        WNDCLASSEX wndClass = { };
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = &WindowProc;
        wndClass.hInstance = hInstance;
        wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYAPP_ICON));
        wndClass.hCursor = LoadCursor(0, IDC_ARROW);
        wndClass.hbrBackground = NULL;
        wndClass.lpszMenuName = 0;
        wndClass.lpszClassName = windowClassName;
    
        if (!RegisterClassEx(&wndClass)) {
            return 1;
        }
    
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };
        HMONITOR monitor = MonitorFromWindow(0, MONITOR_DEFAULTTOPRIMARY);
        if (!GetMonitorInfo(monitor, &monitorInfo)) {
            ASSERT_ERROR(false, "Failed to get monitor info.");
            return 1;
        }
    
        DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_SIZEBOX;
    
        RECT workArea = monitorInfo.rcWork;
        int width = workArea.right - workArea.left;
        int height = workArea.bottom - workArea.top;
    
        // Create the window
        window = CreateWindowEx(0, windowClassName, windowName, dwStyle,
            0, 0, width, height,
            0, 0, hInstance, 0);
    
        ASSERT_ERROR(window, "Failed to create window!");
        ShowWindow(window, SW_HIDE);
    }
    
    // Initalize Renderer
    DX11_InitializeRenderer(window);
    rendererInitialized = true;

    DX11_BeginDrawing();
    ClearBackground(Vector4{0.1f, 0.1f, 0.1f, 1.0f});
    DX11_EndDrawing();
    DX11_GraphicsPresent();
    
    ShowWindow(window, SW_MAXIMIZE);
    UpdateWindow(window);

    // NOTE(roger): Minimum timer resolution, in milliseconds for the application.
    // We use this to sleep the application if there is time left over after updating/rendering the game
    // We call timeEndPeriod at the end of this function.
    timeBeginPeriod(1);

    LARGE_INTEGER PrevEndOfFrameTime;
    QueryPerformanceCounter(&PrevEndOfFrameTime);

    u32 TICKS_PER_SECOND = 60;
    
    // TODO(roger): Read directory and add selection list in program.
    // TODO(roger): Add custom sample instead of requiring reviewers to download a sample.
    InitGame("data/render_me.rsvo");
    
    // Main Loop
    while (true) {
        bool exit = false;
    
        MSG msg = {};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            ProcessEvents(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                exit = true;
            }
        }

        if (exit) {
            break;
        }
        
        TickGame();
        
        // NOTE(roger): Cap the frametime: https://web.archive.org/web/20250125180350/http://www.geisswerks.com/ryan/FAQS/timing.html
        {
            LARGE_INTEGER t;
            QueryPerformanceCounter(&t);

            if (PrevEndOfFrameTime.QuadPart != 0) {
                LARGE_INTEGER frequency;
                QueryPerformanceFrequency(&frequency);
                int ticksToWait = (int)frequency.QuadPart / TICKS_PER_SECOND;
                int done = 0;
                do {
                    QueryPerformanceCounter(&t);

                    int ticksPassed = (int)((int64_t)t.QuadPart - (int64_t)PrevEndOfFrameTime.QuadPart);
                    int ticksLeft = ticksToWait - ticksPassed;

                    if (t.QuadPart < PrevEndOfFrameTime.QuadPart) {
                        done = 1;
                    }

                    if (ticksPassed >= ticksToWait) {
                        done = 1;
                    }

                    if (!done) {
                        // if > 0.002s left, do Sleep(1), which will actually sleep some
                        //   steady amount, probably 1-2 ms,
                        //   and do so in a nice way (cpu meter drops; laptop battery spared).
                        // otherwise, do a few Sleep(0)'s, which just give up the timeslice,
                        //   but don't really save cpu or battery, but do pass a tiny
                        //   amount of time.
                        if (ticksLeft > (int)frequency.QuadPart*2/1000) {
                            Sleep(1);
                        } else {
                            for (int i=0; i<10; i++) {
                                Sleep(0);  // causes thread to give up its timeslice
                            }
                        }
                    }
                } while (!done);
            }
            
            PrevEndOfFrameTime = t;
        }
    }

    timeEndPeriod(1);
    return 0;
}

void CenterWindow(HWND window) {
    RECT windowRect;
    GetWindowRect(window, &windowRect);
    RECT workArea; //Screen area excluding the taskbar
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int posX = (workArea.right - workArea.left - (windowRect.right - windowRect.left)) / 2;
    int posY = (workArea.bottom - workArea.top - (windowRect.bottom - windowRect.top)) / 2;
    SetWindowPos(window, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

Vector2 GetClientSize() {
    RECT clientRect;
    GetClientRect(window, &clientRect);
    float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
    float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);
    return {clientWidth, clientHeight};
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC deviceContext;

    switch (message) {
        case WM_ACTIVATEAPP: {
            isWindowActive = (BOOL)wParam;
        } break;
    
        case WM_CREATE: {
            isWindowActive = true;
            CenterWindow(window);
        } break;

        case WM_PAINT: {
            deviceContext = BeginPaint(window, &ps);
            EndPaint(window, &ps);
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        
        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE: {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);

            bool canResize = width != 0 && height != 0;
            canResize &= (wParam != SIZE_MINIMIZED);

            if (canResize && rendererInitialized) {
                // TODO(roger): The renderer should have a different function that wraps
                // the backend (DX11) call and handles screen textures.
                ResizeGraphics(width, height, 0, 0);
            }
        } break;

        default: {
            return DefWindowProc(window, message, wParam, lParam);
        }
    }

    return 0;
}