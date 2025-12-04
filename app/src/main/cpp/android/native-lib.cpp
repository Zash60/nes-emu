#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include "../core/Nes.h"
extern const uint32_t* GetRawPixelBuffer();
extern void SetKeyState(int scancode, bool pressed);
static std::unique_ptr<Nes> g_nes;
extern "C" JNIEXPORT void JNICALL Java_com_example_nesemu_NesEmulator_init(JNIEnv* env, jobject) { g_nes.reset(new Nes()); g_nes->Initialize(); }
extern "C" JNIEXPORT void JNICALL Java_com_example_nesemu_NesEmulator_loadRom(JNIEnv* env, jobject, jstring p) { if(!g_nes)return; const char* c=env->GetStringUTFChars(p,0); g_nes->LoadRom(c); g_nes->Reset(); env->ReleaseStringUTFChars(p,c); }
extern "C" JNIEXPORT void JNICALL Java_com_example_nesemu_NesEmulator_runFrame(JNIEnv* env, jobject, jobject bmp, jboolean rew) {
    if(!g_nes)return; g_nes->RewindSaveStates(rew); g_nes->ExecuteFrame(false);
    void* px; if(AndroidBitmap_lockPixels(env,bmp,&px)<0)return;
    memcpy(px, GetRawPixelBuffer(), 256*240*4); AndroidBitmap_unlockPixels(env,bmp);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_nesemu_NesEmulator_setButtonState(JNIEnv* env, jobject, jint id, jboolean p) {
    int map[] = {4,22,43,40,82,81,80,79}; // A,B,Sel,Str,Up,Dn,L,R
    if(id>=0 && id<8) SetKeyState(map[id], p);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_nesemu_NesEmulator_saveState(JNIEnv* env, jobject, jboolean s) { if(g_nes)g_nes->SerializeSaveState(s); }
