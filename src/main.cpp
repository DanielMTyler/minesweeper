/*
    ====================================
     Copyright (C) 2020 Daniel M Tyler.
     This file is part of Minesweeper.
    ====================================
*/

#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS 1
#elif defined(__linux__)
    #define OS_LINUX 1
#else
    #error Unknown OS.
#endif

#if defined(__clang__)
    #define COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define COMPILER_GCC 1
#elif defined(__MINGW32__) || defined(__MINGW64__)
    #define COMPILER_MINGW 1
#elif defined(_MSC_VER)
    #define COMPILER_VISUAL_C
#else
    #error Unknown Compiler.
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC) || defined(COMPILER_MINGW)
    #if defined(__i386__)
        #define ARCH_X86
    #elif defined(__x86_64__)
        #define ARCH_X64
    #else
        #error Unknown CPU Architecture.
    #endif
#elif defined(COMPILER_VISUAL_C)
    #if defined(_M_IX86)
        #define ARCH_X86
    #elif defined(_M_X64)
        #define ARCH_X64
    #else
        #error Unknown CPU Architecture.
    #endif
#else
    #error Unknown Compiler.
#endif


#include <cstddef> // (u)int(8/16/32/64)_t
#include <cstdint> // size_t
#include <cstring> // memset
#include <string>  // string

#include <SDL.h>
#ifdef OS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif
#include <exception> // exception
#include <fstream>
#include <iostream>


#define KIBIBYTES(v) ((v) * 1024LL)
#define MEBIBYTES(v) (KIBIBYTES(v) * 1024LL)
#define GIBIBYTES(v) (MEBIBYTES(v) * 1024LL)

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define ZERO_STRUCT(s) std::memset(&(s), 0, sizeof(s))


typedef std::int8_t int8;
typedef std::int16_t int16;
typedef std::int32_t int32;
typedef std::int64_t int64;

typedef std::uint8_t uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

typedef float real32;
typedef double real64;

typedef std::size_t MemoryIndex;
typedef std::size_t MemorySize;


struct ResultBool
{
    bool result;
    std::string error;
    
    explicit operator bool() const { return result; }
};



#ifdef OS_WINDOWS
    // Format GetLastError() and put it into msg.
    // Returns 0 on success or the error code.
    DWORD WindowsFormatLastError(std::string& msg)
    {
        DWORD e = GetLastError();
        if (!e)
            return e;
        
        char* rawMsg = nullptr;
        DWORD s = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                nullptr, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&rawMsg, 0, nullptr);
        if (!s)
            return e;
        
        msg = rawMsg;
        LocalFree(rawMsg);
        return e;
    }

    static HANDLE g_windowsSingleInstanceMutex = nullptr;

    bool VerifySingleInstanceInit()
    {
        SDL_assert(!g_windowsSingleInstanceMutex);
        
        const char* name = "DanielMTyler/Minesweeper/VerifySingleInstance";
        
        g_windowsSingleInstanceMutex = CreateMutex(nullptr, true, name);
        if (g_windowsSingleInstanceMutex && GetLastError() != ERROR_SUCCESS)
        {
            // GetLastError() should be ERROR_ALREADY_EXISTS || ERROR_ACCESS_DENIED.
            g_windowsSingleInstanceMutex = nullptr;
            const char* msg = "Another instance of Minesweeper is already running.";
            const char* title = "Minesweeper Is Already Running";
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, msg, nullptr);
            std::cerr << msg << std::endl;
            return false;
        }
        
        return true;
    }

    void VerifySingleInstanceCleanup()
    {
        if (g_windowsSingleInstanceMutex)
        {
            ReleaseMutex(g_windowsSingleInstanceMutex);
            g_windowsSingleInstanceMutex = nullptr;
        }
    }
#else
    #error Only Windows is supported right now.
#endif

#define LOG_INFO(msg) std::cout << (msg) << "\n";
#define LOG_WARN(msg) std::cout << "Warning: " << (msg) << "\n";
#define LOG_FAIL(msg) std::cout << "Failure: " << (msg) << "\n";


/*
    Game modes:
        Beginner: 10 mines @ 8x8, 9x9, or 10x10.
        Intermediate: 40 mines @ 13x15 or 16x16.
        Expert: 99 mines @ 16x30 or 30x16.
*/


static g_dataPath = "data/";


bool AppInit()
{
    if (!VerifySingleInstanceInit())
        return false;
    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        LOG_FAIL("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    LOG_INFO("Initalized SDL.");
    
    return true;
}

void AppCleanup()
{
    SDL_Quit();
    VerifySingleInstanceCleanup();
}

int AppLoop()
{
    bool quit = false;
    while (!quit)
    {
        quit = true;
        SDL_Delay(1);
    }
    
    return 0;
}

// WARNING: SDL 2 requires this function signature; changing it will give "undefined reference to SDL_main" linker errors.
int main(int argc, char* argv[])
{
    int ret = 1;
    
    try
    {
        if (AppInit())
            ret = AppLoop();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    
    AppCleanup();
    return ret;
}
