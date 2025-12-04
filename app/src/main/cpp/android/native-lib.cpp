#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <android/log.h>
#include "../core/Nes.h"
#include "../core/System.h"

// SDL Scancodes manuais para mapear input
#define SCANCODE_A 4
#define SCANCODE_B 22
#define SCANCODE_UP 82
#define SCANCODE_DOWN 81
#define SCANCODE_LEFT 80
#define SCANCODE_RIGHT 79
#define SCANCODE_SELECT 43
#define SCANCODE_START 40

extern const uint32_t* GetRawPixelBuffer();
extern void SetKeyState(int scancode, bool pressed);

static std::unique_ptr<Nes> g_nes;
static bool g_isRomLoaded = false; // Flag de segurança

extern "C" JNIEXPORT void JNICALL
Java_com_example_nesemu_NesEmulator_init(JNIEnv* env, jobject /*this*/) {
    g_nes = std::unique_ptr<Nes>(new Nes());
    g_nes->Initialize();
    g_isRomLoaded = false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nesemu_NesEmulator_loadRom(JNIEnv* env, jobject /*this*/, jstring filePath) {
    if (!g_nes) return;
    const char* path = env->GetStringUTFChars(filePath, 0);
    
    // Tenta carregar a ROM
    g_nes->LoadRom(path);
    g_nes->Reset();
    
    // Assume sucesso se chegou até aqui sem exceção C++
    g_isRomLoaded = true;
    
    env->ReleaseStringUTFChars(filePath, path);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nesemu_NesEmulator_runFrame(JNIEnv* env, jobject /*this*/, jobject bitmap, jboolean rewind) {
    // Só roda se emulador existe e ROM foi carregada
    if (!g_nes || !g_isRomLoaded) return;

    g_nes->RewindSaveStates(rewind);
    g_nes->ExecuteFrame(false);

    void* bitmapPixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels) < 0) return;

    const uint32_t* src = GetRawPixelBuffer();
    // NES 256x240
    memcpy(bitmapPixels, src, 256 * 240 * 4);

    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nesemu_NesEmulator_setButtonState(JNIEnv* env, jobject /*this*/, jint buttonId, jboolean pressed) {
    int scancode = 0;
    switch(buttonId) {
        case 0: scancode = SCANCODE_A; break;
        case 1: scancode = SCANCODE_B; break;
        case 2: scancode = SCANCODE_SELECT; break;
        case 3: scancode = SCANCODE_START; break;
        case 4: scancode = SCANCODE_UP; break;
        case 5: scancode = SCANCODE_DOWN; break;
        case 6: scancode = SCANCODE_LEFT; break;
        case 7: scancode = SCANCODE_RIGHT; break;
    }
    SetKeyState(scancode, pressed);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nesemu_NesEmulator_saveState(JNIEnv* env, jobject /*this*/, jboolean save) {
    if (g_nes && g_isRomLoaded) {
        g_nes->SerializeSaveState(save);
    }
}
