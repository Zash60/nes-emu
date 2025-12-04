#include "System.h"
#include "IO.h"
#include <SDL.h>
#include <chrono>
#include <cstring> // Necessário para strlen

namespace System
{
    const char* GetAppDirectory()
    {
        static char appDir[2048] = { 0 };
        
        // Lazily build the app directory
        if (appDir[0] == 0)
        {
            // No Android com nosso Fake SDL, isso retorna string vazia
            char* basePath = SDL_GetBasePath();
            std::string temp = basePath ? basePath : "";
            if (basePath) SDL_free(basePath);

            // Se vazio (Android fake), usa um caminho seguro padrão ou vazio
            if (temp.empty()) {
                // No Android, caminhos são gerenciados pelo Java/Context, 
                // mas para evitar crash no C++, deixamos vazio ou "."
                temp = "."; 
            }

            auto npos = temp.find(APP_NAME);
            if (npos != std::string::npos)
            {
                temp.copy(appDir, npos + strlen(APP_NAME) + 1, 0);
            }
            else
            {
                // Garante que cabe no buffer
                size_t len = temp.length();
                if (len >= sizeof(appDir)) len = sizeof(appDir) - 1;
                temp.copy(appDir, len, 0);
                appDir[len] = '\0';
            }
        }

        return appDir;
    }

    void Sleep(uint32 ms)
    {
        SDL_Delay(ms);
    }

    float64 GetTimeSec()
    {
        static Uint64 start = SDL_GetPerformanceCounter();
        return static_cast<float64>(SDL_GetPerformanceCounter() - start) / SDL_GetPerformanceFrequency();
    }
}

// Implementações específicas de plataforma

#if PLATFORM_WINDOWS

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#undef ARRAYSIZE 
#include <Windows.h>
#include <commdlg.h>
#include <conio.h>
#include <cstdio>

#undef MessageBox
#undef CreateDirectory

namespace System
{
    bool CreateDirectory(const char* directory)
    {
        return ::CreateDirectoryA(directory, NULL) != FALSE;
    }

    void DebugBreak()
    {
        ::DebugBreak();
    }

    void MessageBox(const char* title, const char* message)
    {
        printf("%s: %s\n", title, message);
        ::MessageBoxA(::GetActiveWindow(), message, title, MB_OK);
    }
    
    bool SupportsOpenFileDialog()
    {
        return true;
    }

    bool OpenFileDialog(std::string& fileSelected, const char* title, const char* filter)
    {
        char file[_MAX_PATH] = "";
        char currDir[_MAX_PATH] = "";
        ::GetCurrentDirectoryA(sizeof(currDir), currDir);

        OPENFILENAMEA ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = file;
        ofn.nMaxFile = sizeof(file);
        ofn.lpstrTitle = title;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 0;
        ofn.lpstrInitialDir = currDir;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (::GetOpenFileNameA(&ofn) == TRUE)
        {
            fileSelected = ofn.lpstrFile;
            return true;
        }
        return false;
    }
}

// CORREÇÃO: Removido caractere unicode invisível na linha abaixo
#elif defined(__ANDROID__) || PLATFORM_LINUX || PLATFORM_MAC

#include <sys/stat.h>
#include <unistd.h>
#include <cassert>

namespace System
{
    bool CreateDirectory(const char* directory)
    {
        struct stat st = {0};
        if (stat(directory, &st) == -1) {
            return mkdir(directory, 0777) == 0;
        }
        return false;
    }

    void DebugBreak()
    {
        // No Android/Linux, assert(false) ou raise(SIGTRAP)
        // assert(false); 
    }

    void MessageBox(const char* title, const char* message)
    {
        printf("%s: %s\n", title, message);
    }
    
    bool SupportsOpenFileDialog()
    {
        return false;
    }

    bool OpenFileDialog(std::string& fileSelected, const char* title, const char* filter)
    {
        return false;
    }
}

#else
#error "Implement for current platform"
#endif
