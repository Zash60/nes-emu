#pragma once
#include "SDL_scancode.h"
#include "SDL_render.h"

// Tipos básicos
typedef unsigned char Uint8;
typedef unsigned int Uint32;

// Áudio Dummy
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

// Funções Gerais Dummy
inline void SDL_InitSubSystem(unsigned int flags) {}
inline void SDL_QuitSubSystem(unsigned int flags) {}
inline int SDL_OpenAudioDevice(const char* device, int iscapture, const SDL_AudioSpec* desired, void* obtained, int allowed_changes) { return 1; }
inline void SDL_CloseAudioDevice(int dev) {}
inline void SDL_PauseAudioDevice(int dev, int pause_on) {}
inline void SDL_LockAudioDevice(int dev) {}
inline void SDL_UnlockAudioDevice(int dev) {}
inline void SDL_zero(SDL_AudioSpec x) {}
inline const char* SDL_GetError() { return ""; }

#define SDL_INIT_AUDIO 0x00000010
#define SDL_AUDIO_ALLOW_ANY_CHANGE 0
typedef int SDL_AudioDeviceID;

#define SDL_MAIN_HANDLED
