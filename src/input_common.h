#ifndef INPUT_COMMON_H
#define INPUT_COMMON_H

#ifndef SERIALIZE
    #define SERIALIZE()
#endif

#ifndef USE_FULL_NAME
    #define USE_FULL_NAME()
#endif

SERIALIZE() enum InputCode : u32 {
    KEY_NONE = 0,
    
    KEYBOARD_BEGIN,
    
    KEY_SPACE        = 0x20,
    KEY_ESCAPE       = 0x1B,
    KEY_DELETE       = 0x2E,
    KEY_SHIFT        = 0x10,
    KEY_BACK         = 0x08, 
    KEY_TAB          = 0x09,
    KEY_ENTER        = 0x0D,
    KEY_CONTROL      = 0x11,
    KEY_ALT          = 0x12,
    KEY_PAUSE        = 0x13,
    KEY_CAPS_LOCK    = 0x14,
    KEY_PAGE_UP      = 0x21,
    KEY_PAGE_DOWN    = 0x22,
    KEY_END          = 0x23,
    KEY_HOME         = 0x24,
    KEY_LEFT         = 0x25, 
    KEY_UP           = 0x26,
    KEY_RIGHT        = 0x27,
    KEY_DOWN         = 0x28,
    KEY_PRINT_SCREEN = 0x2C,
    KEY_INSERT       = 0x2D,
    
    KEY_0 = 0x30,
    KEY_1 = 0x31,
    KEY_2 = 0x32,
    KEY_3 = 0x33,
    KEY_4 = 0x34,
    KEY_5 = 0x35,
    KEY_6 = 0x36,
    KEY_7 = 0x37,
    KEY_8 = 0x38,
    KEY_9 = 0x39,
    KEY_A = 0x41,
    KEY_B = 0x42,
    KEY_C = 0x43,
    KEY_D = 0x44,
    KEY_E = 0x45,
    KEY_F = 0x46,
    KEY_G = 0x47,
    KEY_H = 0x48,
    KEY_I = 0x49,
    KEY_J = 0x4A,
    KEY_K = 0x4B,
    KEY_L = 0x4C,
    KEY_M = 0x4D,
    KEY_N = 0x4E,
    KEY_O = 0x4F,
    KEY_P = 0x50,
    KEY_Q = 0x51,
    KEY_R = 0x52,
    KEY_S = 0x53,
    KEY_T = 0x54,
    KEY_U = 0x55,
    KEY_V = 0x56,
    KEY_W = 0x57,
    KEY_X = 0x58,
    KEY_Y = 0x59,
    KEY_Z = 0x5A,
    
    KEY_F1  = 0x70,
    KEY_F2  = 0x71,
    KEY_F3  = 0x72,
    KEY_F4  = 0x73,
    KEY_F5  = 0x74,
    KEY_F6  = 0x75,
    KEY_F7  = 0x76,
    KEY_F8  = 0x77,
    KEY_F9  = 0x78,
    KEY_F10 = 0x79,
    KEY_F11 = 0x7A,
    KEY_F12 = 0x7B,
    
    KEY_NUMPAD_0 = 0x60,
    KEY_NUMPAD_1 = 0x61,
    KEY_NUMPAD_2 = 0x62,
    KEY_NUMPAD_3 = 0x63,
    KEY_NUMPAD_4 = 0x64,
    KEY_NUMPAD_5 = 0x65,
    KEY_NUMPAD_6 = 0x66,
    KEY_NUMPAD_7 = 0x67,
    KEY_NUMPAD_8 = 0x68,
    KEY_NUMPAD_9 = 0x69,
    
    KEY_SEMICOLON = 0xBA,
    KEY_EQUALS    = 0xBB,
    KEY_COMMA     = 0xBC,
    KEY_PLUS      = 0xBB,
    KEY_MINUS     = 0xBD,
    KEY_PERIOD    = 0xBE,
    KEY_SLASH     = 0xBF,
    KEY_TILDE     = 0xC0,
    
    KEYBOARD_END,
    
    USE_FULL_NAME() MOUSE_BEGIN   = 0x1000,
    USE_FULL_NAME() MOUSE_LEFT,
    USE_FULL_NAME() MOUSE_RIGHT, 
    USE_FULL_NAME() MOUSE_MIDDLE,
    USE_FULL_NAME() MOUSE_END,

    GP_BEGIN = 0x3000,
    GP_A,
    GP_B,
    GP_X,
    GP_Y,
    GP_LEFT_TRIGGER,
    GP_RIGHT_TRIGGER,
    GP_LEFT_BUMPER,
    GP_RIGHT_BUMPER,
    GP_BACK,
    GP_START,
    GP_LEFT_THUMB_PRESS,
    GP_RIGHT_THUMB_PRESS,
    
    GP_LEFTSTICK_UP,
    GP_LEFTSTICK_DOWN,
    GP_LEFTSTICK_LEFT,
    GP_LEFTSTICK_RIGHT,
    GP_DPAD_UP,
    GP_DPAD_DOWN,
    GP_DPAD_LEFT,
    GP_DPAD_RIGHT,
    GP_END,
    
    INPUT_COUNT
};

bool  gamepadActive;
bool  prevGamepadActive;
float gamepadLeftStickX;   
float gamepadLeftStickY;   
float gamepadRightStickX;  
float gamepadRightStickY;  
float gamepadLeftTrigger;  
float gamepadRightTrigger;

bool IsInputDown(InputCode action);
bool IsInputUp(InputCode action);
bool IsInputPressed(InputCode action);
bool IsInputReleased(InputCode action);
bool IsAnyInputPressed();
char* GetInputCodeName(InputCode input);
bool GetAnyInputPressed(InputCode* code);

int mouseScrollDelta;
bool currentInputState[INPUT_COUNT];
bool previousInputState[INPUT_COUNT];
bool ignoreUntilRelease[INPUT_COUNT];
bool osCursorVisible = true;
bool isWindowActive;

void ShowOsCursor();
void HideOsCursor();
Vector2 GetMousePosition();
void LockCursor();
void UnlockCursor();
void CenterCursorInWindow();

void FlushInput();
void HandleTransitionInput();
void ProcessGamepad();

#define MAX_SHORT 32768

Vector2 NormalizeStick(short rawX, short rawY, short deadzone) {
    Vector2 direction = { (float)rawX / MAX_SHORT, (float)rawY / MAX_SHORT };

    float magnitude = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (magnitude < (float)deadzone / MAX_SHORT) {
        return {};
    }

    // Remap magnitude from [deadzone, 1.0] → [0, 1.0]
    float normalizedMag = (magnitude - ((float)deadzone / MAX_SHORT)) / (1.0f - ((float)deadzone / MAX_SHORT));
    float scale = normalizedMag / magnitude;
    return direction * scale;
}

float NormalizeTrigger(int v) {
    float f = (v > 0 ? (float)v : 0.0f) / 32767.0f;
    if (f < 0.0f) {
        f = 0.0f;
    }
    else if (f > 1.0f) {
         f = 1.0f;
    }
    return f;
}

bool GetAnyGamepadInputPressed(InputCode* outCode) {
    for (int i = GP_BEGIN; i < GP_END; ++i) {
        if (currentInputState[i]) {
            *outCode = (InputCode)i;
            return true;
        }
    }
    return false;
}

#endif