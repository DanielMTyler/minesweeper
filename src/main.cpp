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
#include <list>    // list
#include <memory>  // make_shared, shared_ptr, weak_ptr
#include <string>  // string

/*
    WARNING: SDL.h must be included before windows.h or you'll get compile errors like this:
        In file included from ..\..\src\platform_win64.cpp:143:
        In file included from ..\..\src/core.cpp:10:
        In file included from ..\..\deps\include\SDL2\SDL.h:38:
        In file included from ..\..\deps\include\SDL2/SDL_cpuinfo.h:59:
        In file included from C:\Program Files\LLVM\lib\clang\10.0.0\include\intrin.h:12:
        In file included from C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\x86_64-w64-mingw32\include\intrin.h:41:
        C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\x86_64-w64-mingw32\include\psdk_inc/intrin-impl.h:1781:18: error: redefinition of '__builtin_ia32_xgetbv' as different kind of symbol
        unsigned __int64 _xgetbv(unsigned int);
    
    WARNING: glad and spdlog both include windows.h and cause the problem above.
 */
#include <SDL.h>

#ifdef NDEBUG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#else
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/spdlog.h>



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

typedef float DeltaTime;

typedef spdlog::logger          Logger;
typedef std::shared_ptr<Logger> StrongLoggerPtr;
typedef std::weak_ptr<Logger>   WeakLoggerPtr;


struct ResultBool
{
    bool result;
    std::string error;
    
    explicit operator bool() const { return result; }
};

const char* TrueFalseToStr(bool b)
{
    return (b ? "True" : "False");
}

const char* OnOffToStr(bool b)
{
    return (b ? "On" : "Off");
}


uint32 CheckedUInt64ToUInt32(uint64 v)
{
    SDL_assert_release(v <= 0xFFFFFFFF);
    uint32 r = (uint32)v;
    return r;
}

#endif // GLOBAL_HPP_INCLUDED.







/*
    ==================================
    Copyright (C) 2020 Daniel M Tyler.
      This file is part of Ellie.
    ==================================
*/

#ifndef PLATFORM_HPP_INCLUDED
#define PLATFORM_HPP_INCLUDED

#include "global.hpp"
#include <SDL.h>
#include <string>



/// These defines are per OS below.
// #define PATH_SEPARATOR "/"
// #define SHARED_LIBRARY_PREFIX "lib"
// #define SHARED_LIBRARY_EXTENSION "so"



ResultBool  RetrieveCWD   (std::string& cwd);

/// Returns true if we're alone OR if a failure occurs.
/// Displays an error message to the user if another instance is running.
bool VerifySingleInstanceInit();
void VerifySingleInstanceCleanup();



#ifdef OS_WINDOWS


// @todo Support paths > MAX_PATH via Unicode/path prefixes.


#define PATH_SEPARATOR "\\"
#define SHARED_LIBRARY_PREFIX ""
#define SHARED_LIBRARY_EXTENSION "dll"


#define WIN32_LEAN_AND_MEAN
#include <windows.h>


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


ResultBool RetrieveCWD(std::string& cwd)
{
    ResultBool r;
    
    char cwdBuf[MAX_PATH];
    if (!GetCurrentDirectory(MAX_PATH, cwdBuf))
    {
        std::string msg;
        DWORD e = WindowsFormatLastError(msg);
        if (!msg.empty())
            r.error = "GetCurrentDirectory failed: " + msg;
        else
            r.error = "GetCurrentDirectory failed: GetLastError()=" + std::to_string(e);
        
        r.result = false;
        return r;
    }
    
    cwd = cwdBuf;
    // GetCurrentDirectory doesn't add a path separator at the end.
    cwd += PATH_SEPARATOR;
    r.result = true;
    return r;
}


HANDLE g_windowsSingleInstanceMutex = nullptr;

bool VerifySingleInstanceInit()
{
    SDL_assert(!g_windowsSingleInstanceMutex);
    
    const char* name = ORGANIZATION "/" PROJECT_NAME "/VerifySingleInstance";
    
    g_windowsSingleInstanceMutex = CreateMutex(nullptr, true, name);
    if (g_windowsSingleInstanceMutex && GetLastError() != ERROR_SUCCESS)
    {
        // GetLastError() should be ERROR_ALREADY_EXISTS || ERROR_ACCESS_DENIED.
        g_windowsSingleInstanceMutex = nullptr;
        const char* msg = "Another instance of " PROJECT_NAME " is already running.";
        const char* title = PROJECT_NAME " Is Already Running";
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
    #error Unknown platform.
#endif // OS_WINDOWS.

#endif // PLATFORM_HPP_INCLUDED.












/*
    ==================================
    Copyright (C) 2020 Daniel M Tyler.
      This file is part of Ellie.
    ==================================
*/

#include "global.hpp"
#include <SDL.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <exception> // exception
#include <iostream>
#include <memory> // make_shared, shared_ptr
#include <string>
#include <vector>
#include "platform.hpp"
#include "resources.hpp"
#include "tasks.hpp"



bool AppInitSavePathTask(StrongTaskPtr t)
{
    // Must SDL_Init before SDL_GetPrefPath.
    if (SDL_Init(0) < 0)
    {
        std::cerr << "Failed to initialze minimal SDL: " << SDL_GetError() << "." << std::endl;
        return false;
    }
    
    char* prefPath = SDL_GetPrefPath(ORGANIZATION, PROJECT_NAME);
    if (!prefPath)
    {
        std::cerr << "Failed to get save path: " << SDL_GetError() << "." << std::endl;
        return false;
    }
    t->child->userData = new std::string(prefPath);
    SDL_free(prefPath);
    prefPath = nullptr;
    
    return true;
}

bool AppInitLogTask(StrongTaskPtr t)
{
    const std::string& savePath = *((std::string*)t->userData);
    t->child->userData = t->userData;
    
    try
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(savePath + PROJECT_NAME + ".log", true);
        
        consoleSink->set_pattern("[%^%L%$] %v");
        fileSink->set_pattern("%T [%L] %v");
        
        #ifdef NDEBUG
            consoleSink->set_level(spdlog::level::info);
            fileSink->set_level(spdlog::level::info);
        #else
            consoleSink->set_level(spdlog::level::trace);
            fileSink->set_level(spdlog::level::trace);
        #endif
        
        std::vector<spdlog::sink_ptr> loggerSinks;
        loggerSinks.push_back(consoleSink);
        loggerSinks.push_back(fileSink);
        StrongLoggerPtr logger = std::make_shared<Logger>("default", loggerSinks.begin(), loggerSinks.end());
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        delete (std::string*)t->userData;
        std::cerr << "Failed to initialize logger: " << ex.what() << "." << std::endl;
        return false;
    }
    
    #ifndef NDEBUG
        SPDLOG_WARN("Debug Build.");
    #endif
    
    return true;
}

bool AppInitSDLTask(StrongTaskPtr t)
{
    t->child->userData = t->userData;
    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        delete (std::string*)t->userData;
        SPDLOG_CRITICAL("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    SPDLOG_INFO("Initalized SDL.");
    
    return true;
}

bool AppInitPathsTask(StrongTaskPtr t)
{
    std::string savePath = *((std::string*)t->userData);
    delete (std::string*)t->userData;
    t->userData = nullptr;
    
    // exePathBuf will end with a path separator, which is what we want.
    char* exePathBuf = SDL_GetBasePath();
    if (!exePathBuf)
    {
        SPDLOG_CRITICAL("Failed to get executable path: {}.", SDL_GetError());
        return false;
    }
    std::string exePath = exePathBuf;
    SDL_free(exePathBuf);
    exePathBuf = nullptr;
    
    std::string cwdPath;
    ResultBool r = RetrieveCWD(cwdPath);
    if (!r)
    {
        SPDLOG_CRITICAL("Failed to get the current working directory: {}.", r.error);
        return false;
    }
    
    // Find the data folder in cwdPath, exePath, or "<cwdPath>/../../release/" in debug builds.
    std::string pathSep = PATH_SEPARATOR;
    std::string releasePath = cwdPath;
    std::string dataPath = releasePath + "data" + pathSep;
    if (!FolderExists(dataPath))
    {
        releasePath = exePath;
        dataPath = releasePath + "data" + pathSep;
        if (!FolderExists(dataPath))
        {
            #ifdef NDEBUG
                SPDLOG_CRITICAL("The data folder wasn't found in the current working directory ({}) or the executable directory ({}).", cwdPath, exePath);
                return false;
            #else
                // Move cwdPath up 2 directories.
                releasePath = cwdPath.substr(0, cwdPath.size()-1);
                releasePath = releasePath.substr(0, releasePath.find_last_of(pathSep));
                releasePath = releasePath.substr(0, releasePath.find_last_of(pathSep));
                releasePath += pathSep + "release" + pathSep;
                dataPath = releasePath + "data" + pathSep;
                if (!FolderExists(dataPath))
                {
                    SPDLOG_CRITICAL("The data folder wasn't found in the current working directory ({}), the executable directory ({}), or \"<cwd>../../release/\" ({}).", cwdPath, exePath, releasePath);
                    return false;
                }
            #endif
        }
    }
    
    SPDLOG_INFO("Save path: {}.", savePath);
    SPDLOG_INFO("Data path: {}.", dataPath);
    if (!ResManInit(dataPath, savePath))
        return false;
    
    return true;
}

bool AppInit()
{
    if (!VerifySingleInstanceInit())
        return false;
    
    TaskManager tm;
    TaskManInit(tm);
    StrongTaskPtr t = TaskManAttachTask(tm, TaskCreate(AppInitSavePathTask));
    t = TaskAttachChild(t, TaskCreate(AppInitLogTask));
    t = TaskAttachChild(t, TaskCreate(AppInitSDLTask));
    t = TaskAttachChild(t, TaskCreate(AppInitPathsTask));
    while (TaskManNumTasks(tm))
        TaskManUpdate(tm, 0.0f);
    TaskCount numFailed = TaskManNumFailed(tm);
    TaskManCleanup(tm);
    if (numFailed)
        return false;
    
    return true;
}

void AppCleanup()
{
    ResManCleanup();
    SDL_Quit();
    spdlog::shutdown();
    VerifySingleInstanceCleanup();
}

int AppLoop()
{
    // @todo dt and timeDilation should be in the logic system.
    bool quit = false;
    // Time Dilation is how fast time moves, e.g., 0.5f means time is 50% slower than normal.
    DeltaTime timeDilation = 1.0f;
    uint64 dtNow = SDL_GetPerformanceCounter();
    uint64 dtLast = 0;
    DeltaTime dtReal = 0.0f;
    DeltaTime dt = 0.0f;
    while (!quit)
    {
        dtLast = dtNow;
        dtNow = SDL_GetPerformanceCounter();
        dtReal = (DeltaTime)(dtNow - dtLast) * 1000.0f / (DeltaTime)SDL_GetPerformanceFrequency();
        dt = dtReal * timeDilation;
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
