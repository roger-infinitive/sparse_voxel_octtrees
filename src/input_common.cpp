#include "input_common.h"

void FlushInput() {
    mouseScrollDelta = 0;
    memcpy(previousInputState, currentInputState, sizeof(currentInputState));
}

bool IsInputDown(InputCode code) {
    return currentInputState[code];
}

bool IsInputUp(InputCode code) {
    return !currentInputState[code];
}

bool IsInputPressed(InputCode code) {
    return currentInputState[code] && !previousInputState[code];
}

bool IsInputReleased(InputCode code) {
    return !currentInputState[code] && previousInputState[code];
}

bool IsAnyInputPressed() {
    for (u32 i = 0; i < INPUT_COUNT; i++) {
        if (currentInputState[i] && !previousInputState[i]) {
            return true;
        }
    }
    return false;
}

bool GetAnyInputPressed(InputCode* code) {
    for (u32 i = 0; i < INPUT_COUNT; i++) {
        if (currentInputState[i] && !previousInputState[i]) {
            *code = (InputCode)i;
            return true;
        }
    }
    return false;
}

void HandleTransitionInput() {
    for (u32 i = 0; i < INPUT_COUNT; i++) {
        if (currentInputState[i] && !previousInputState[i]) {
            currentInputState[i] = false;
            ignoreUntilRelease[i] = true;
        }
    }
}

void ProcessGamepadButton(InputCode code, bool condition) {
    if (ignoreUntilRelease[code]) {
        if (!condition) {
            ignoreUntilRelease[code] = false;
        }
    } else {
        currentInputState[code] = condition;
    }
}

void ProcessKeyboardAndMouseActivity() {
    bool keyboardActive = false;
    for (int i = KEYBOARD_BEGIN; i < KEYBOARD_END; i++) {
        if (currentInputState[i] && !previousInputState[i]) {
            keyboardActive = true;
            break;
        }
    }
    
    for (int i = MOUSE_BEGIN; i < MOUSE_END; i++) {
        if (currentInputState[i] && !previousInputState[i]) {
            keyboardActive = true;
            break;
        }
    }
    
    Vector2 mousePosition = GetMousePosition();
    local_persist Vector2 lastMousePosition = GetMousePosition();
    local_persist Vector2 mouseDelta = {};

    mouseDelta = mousePosition - lastMousePosition;
    if (SqrMagnitude(mouseDelta) > SQUARE(20)) {
        keyboardActive = true;
    }
    
    if (Abs(mouseScrollDelta) > 0) {
        keyboardActive = true;
    }

    if (keyboardActive) {
        gamepadActive = false;
    }
    
    lastMousePosition = mousePosition;
}

