#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Definição de tipos básicos do SDL necessários
typedef uint8_t Uint8;

// Estrutura SDL_Rect (ESSENCIAL para corrigir o erro 'unknown type name')
typedef struct SDL_Rect {
    int x, y;
    int w, h;
} SDL_Rect;

// Estrutura opaca para o Renderer
typedef struct SDL_Renderer SDL_Renderer;

// Funções Dummy (Stubs) para enganar o compilador
// static inline para não dar erro de definição duplicada no linker
static inline int SDL_SetRenderDrawColor(SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return 0;
}

static inline int SDL_RenderFillRect(SDL_Renderer * renderer, const SDL_Rect * rect) {
    return 0;
}

// Outras funções que podem ser chamadas
static inline void SDL_RenderPresent(SDL_Renderer * renderer) {}
static inline void SDL_RenderCopy(SDL_Renderer * renderer, void * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect) {}

#ifdef __cplusplus
}
#endif
