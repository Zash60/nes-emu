#pragma once
#include "SDL_scancode.h"
#include "SDL_render.h"

// Definições básicas para evitar erros
typedef unsigned char Uint8;
typedef unsigned int Uint32;

// Stub para funções de áudio do SDL se aparecerem
typedef uint16_t SDL_AudioFormat;
#define AUDIO_S16 0x8010
typedef void (*SDL_AudioCallback)(void *userdata, Uint8 * stream, int len);
typedef struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    int channels;
    int samples;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

inline void SDL_InitSubSystem(unsigned int) {}
inline void SDL_QuitSubSystem(unsigned int) {}
inline int SDL_OpenAudioDevice(const char*, int, const SDL_AudioSpec*, void*, int) { return 1; }
inline void SDL_CloseAudioDevice(int) {}
inline void SDL_PauseAudioDevice(int, int) {}
inline void SDL_LockAudioDevice(int) {}
inline void SDL_UnlockAudioDevice(int) {}
inline void SDL_zero(SDL_AudioSpec) {}
inline const char* SDL_GetError() { return ""; }
#define SDL_INIT_AUDIO 0x00000010
#define SDL_AUDIO_ALLOW_ANY_CHANGE 0
typedef int SDL_AudioDeviceID;
