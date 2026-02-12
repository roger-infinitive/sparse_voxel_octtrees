#include "input_common.h"
#include <string.h>
#include <xinput.h>

extern HWND window;

void ProcessEvents(MSG* msg) {
    if (!isWindowActive) {
        return;
    }

    u8 keyCode = (u8)msg->wParam;
    switch (msg->message) {
        case WM_KEYDOWN: {
            // Assuming eventData->keyCode corresponds to our InputCode value.
            if (keyCode >= 0 && keyCode < INPUT_COUNT) {
                if (ignoreUntilRelease[keyCode]) {
                    return;
                }
                currentInputState[keyCode] = true;
            }
        } break;
        case WM_KEYUP: {
            if (keyCode >= 0 && keyCode < INPUT_COUNT) {
                currentInputState[keyCode] = false;
                ignoreUntilRelease[keyCode] = false;
            }
        } break;
        case WM_MOUSEWHEEL: {
            mouseScrollDelta = GET_WHEEL_DELTA_WPARAM(msg->wParam);
        } break;
        case WM_LBUTTONDOWN: {
            currentInputState[MOUSE_LEFT] = true;
        } break;
        case WM_LBUTTONUP: {
            currentInputState[MOUSE_LEFT] = false;
        } break;
        case WM_RBUTTONDOWN: {
            currentInputState[MOUSE_RIGHT] = true;
        } break;
        case WM_RBUTTONUP: {
            currentInputState[MOUSE_RIGHT] = false;
        } break;
        case WM_MBUTTONDOWN: {
            currentInputState[MOUSE_MIDDLE] = true;
            SetCapture(window);
        } break;
        case WM_MBUTTONUP: {
            currentInputState[MOUSE_MIDDLE] = false;
            ReleaseCapture();
        } break;
    }
}

void ProcessGamepad() {
    const int gamepadInputCount = (GP_END - GP_BEGIN);
    bool prevGamepadState[gamepadInputCount];
    
    for (int i = 0; i < gamepadInputCount; ++i) {
        prevGamepadState[i] = currentInputState[GP_BEGIN + i];
    }
    
    DWORD dwResult;
    XINPUT_STATE xinputState = {};
    dwResult = XInputGetState(0, &xinputState);
    if (dwResult == ERROR_SUCCESS) {

        // Update gamepad button states
        ProcessGamepadButton(GP_A,                 (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_A));
        ProcessGamepadButton(GP_B,                 (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_B));
        ProcessGamepadButton(GP_X,                 (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_X));
        ProcessGamepadButton(GP_Y,                 (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_Y));
        ProcessGamepadButton(GP_LEFT_BUMPER,       (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER));
        ProcessGamepadButton(GP_RIGHT_BUMPER,      (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER));
        ProcessGamepadButton(GP_BACK,              (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK));
        ProcessGamepadButton(GP_START,             (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_START));
        ProcessGamepadButton(GP_LEFT_THUMB_PRESS,  (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB));
        ProcessGamepadButton(GP_RIGHT_THUMB_PRESS, (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB));
        ProcessGamepadButton(GP_DPAD_UP,           (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP));
        ProcessGamepadButton(GP_DPAD_DOWN,         (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN));
        ProcessGamepadButton(GP_DPAD_LEFT,         (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT));
        ProcessGamepadButton(GP_DPAD_RIGHT,        (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT));
    
        // Process analog inputs
        short rawLX = xinputState.Gamepad.sThumbLX;
        short rawLY = xinputState.Gamepad.sThumbLY;
        short rawRX = xinputState.Gamepad.sThumbRX;
        short rawRY = xinputState.Gamepad.sThumbRY;
    
        short DEADZONE_LS = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        short DEADZONE_RS = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    
        // NormalizeStick (see input_common.h) smoothly handles both components together.
        Vector2 leftStick  = NormalizeStick(rawLX, rawLY, DEADZONE_LS);
        Vector2 rightStick = NormalizeStick(rawRX, rawRY, DEADZONE_RS);
    
        gamepadLeftStickX  = leftStick.x;
        gamepadLeftStickY  = leftStick.y;
        gamepadRightStickX = rightStick.x;
        gamepadRightStickY = rightStick.y;

        ProcessGamepadButton(GP_LEFTSTICK_UP, gamepadLeftStickY > 0.5);
        ProcessGamepadButton(GP_LEFTSTICK_DOWN, gamepadLeftStickY < -0.5);
        ProcessGamepadButton(GP_LEFTSTICK_RIGHT, gamepadLeftStickX > 0.5);
        ProcessGamepadButton(GP_LEFTSTICK_LEFT, gamepadLeftStickX < -0.5);

        gamepadLeftTrigger  = xinputState.Gamepad.bLeftTrigger  / 255.0f;
        gamepadRightTrigger = xinputState.Gamepad.bRightTrigger / 255.0f;
        
        ProcessGamepadButton(GP_LEFT_TRIGGER,  gamepadLeftTrigger > 0);
        ProcessGamepadButton(GP_RIGHT_TRIGGER, gamepadRightTrigger > 0);
    
        if (!gamepadActive) {
            // Check if any gamepad input has changed since the last frame.
            bool anyChanged = false;    
            for (int i = 0; i < gamepadInputCount; ++i) {
                if (currentInputState[GP_BEGIN + i] != prevGamepadState[i]) {
                    anyChanged = true;
                    break;
                }
            }

            if (!anyChanged) {
                anyChanged = gamepadLeftStickX != 0 
                        | gamepadLeftStickY != 0
                        | gamepadRightStickX != 0
                        | gamepadRightStickY != 0;
            }

            // If any gamepad input changed, mark the controller as active.
            gamepadActive = anyChanged;
        }
    } else {
        // Controller not connected
        gamepadActive = false;
    }
}

// IMPORTANT(roger): ShowCursor (win32 api) decrements an internal index! 
// If ShowCursor(0) is called multiple times, then you have to call ShowCursor(1) multiple times to bring it back!
// We protect this with our own osCursorVisible boolean.
                
void ShowOsCursor() {
    if (osCursorVisible) { return; }
    ShowCursor(1);
    osCursorVisible = true;
}

void HideOsCursor() {
    if (!osCursorVisible) { return; }
    ShowCursor(0);
    osCursorVisible = false;
}

Vector2 GetMousePosition() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(window, &cursorPos); // Convert to client-area coordinates

    bool isFinite = IsFinite(cursorPos.x) && IsFinite(cursorPos.y);
    ASSERT_DEBUG(isFinite, "Cursor position should only contain finite coordinates!");

    return { (float)cursorPos.x, (float)cursorPos.y };
}

void UpdateClipCursor(HWND window) {
    RECT rect;
    GetClientRect(window, &rect);
    POINT ul; // Upper left
    POINT lr; // Lower right

    ul.x = rect.left;
    ul.y = rect.top;

    lr.x = rect.right;
    lr.y = rect.bottom;

    // Convert client coordinates to screen coordinates
    ClientToScreen(window, &ul);
    ClientToScreen(window, &lr);

    // Create new rectangle
    RECT newRect;
    newRect.left = ul.x;
    newRect.top = ul.y;
    newRect.right = lr.x;
    newRect.bottom = lr.y;

    ClipCursor(&newRect);
}

void ReleaseCursor() {
    ClipCursor(0);
}

void LockCursor() {
    UpdateClipCursor(window);
}

void UnlockCursor() {
    ReleaseCursor();
}

void CenterCursorInWindow() {
    RECT rect;
    GetClientRect(window, &rect);

    // Find center point in client coordinates
    POINT center = {
        (rect.right - rect.left) / 2,
        (rect.bottom - rect.top) / 2
    };

    // Convert client coordinates to screen coordinates
    ClientToScreen(window, &center);
    SetCursorPos(center.x, center.y);
}