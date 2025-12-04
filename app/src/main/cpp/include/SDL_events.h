#pragma once
typedef struct SDL_Event { int type; } SDL_Event;
inline int SDL_PollEvent(SDL_Event*) { return 0; }
#define SDL_QUIT 0x100
