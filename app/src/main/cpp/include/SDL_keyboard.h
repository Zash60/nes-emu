#pragma once
#include "SDL_scancode.h"
typedef unsigned char Uint8;
// Retorna um array dummy
inline const Uint8* SDL_GetKeyboardState(int*) { 
    static Uint8 state[512] = {0}; 
    return state; 
}
inline int SDL_GetKeyboardFocus() { return 1; }
inline const char* SDL_GetScancodeName(SDL_Scancode) { return ""; }
