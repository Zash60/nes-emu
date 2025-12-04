#include "../core/Input.h"
bool g_keys[512] = {false}; 
namespace Input {
    void Update() {}
    bool KeyDown(SDL_Scancode c) { return (c>=0 && c<512) ? g_keys[c] : false; }
    bool KeyUp(SDL_Scancode c) { return !KeyDown(c); }
    bool KeyPressed(SDL_Scancode c) { return KeyDown(c); }
    bool KeyReleased(SDL_Scancode c) { return KeyUp(c); }
    bool AltDown() { return false; } bool CtrlDown() { return false; } bool ShiftDown() { return false; }
    const char* GetScancodeName(SDL_Scancode c) { return "Key"; }
}
void SetKeyState(int scancode, bool pressed) { if(scancode>=0 && scancode<512) g_keys[scancode] = pressed; }
