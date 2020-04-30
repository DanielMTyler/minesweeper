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
#include <sstream>
#include <cstdlib> // srand, rand
#include <ctime> // time


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


#if 1
    static std::string g_dataPath = "D:/Daniel/Projects/minesweeper/release/data/";
#else
    static std::string g_dataPath = "data/";
#endif

std::stringstream ss;

/*
    Game modes:
        Beginner: 10 mines @ 8x8, 9x9, or 10x10.
        Intermediate: 40 mines @ 13x15 or 16x16.
        Expert: 99 mines @ 16x30 or 30x16.
*/

#define NUM_COLS 30
#define NUM_ROWS 16
#define NUM_MINES 99
#define IMAGE_WIDTH 15
#define IMAGE_HEIGHT IMAGE_WIDTH
#define WINDOW_WIDTH IMAGE_WIDTH*NUM_COLS
#define WINDOW_HEIGHT IMAGE_HEIGHT*NUM_ROWS

static SDL_Window* g_window = nullptr;
static SDL_Surface* g_screenSurface = nullptr;
static SDL_Surface* g_0Surface = nullptr;
static SDL_Surface* g_1Surface = nullptr;
static SDL_Surface* g_2Surface = nullptr;
static SDL_Surface* g_3Surface = nullptr;
static SDL_Surface* g_4Surface = nullptr;
static SDL_Surface* g_5Surface = nullptr;
static SDL_Surface* g_6Surface = nullptr;
static SDL_Surface* g_7Surface = nullptr;
static SDL_Surface* g_8Surface = nullptr;
static SDL_Surface* g_flagSurface = nullptr;
static SDL_Surface* g_guessSurface = nullptr;
static SDL_Surface* g_pressedSurface = nullptr;
static SDL_Surface* g_raisedSurface = nullptr;
static SDL_Surface* g_explodedSurface = nullptr;

struct Cell
{
    bool isMine = false;
    bool isRevealed = false;
    bool isFlagged = false;
    bool isGuessed = false;
    bool isExploded = false; // Player clicked the mine and lost.
    bool isPressed = false; // Player is holding left-click on this cell or is using middle-click.
    uint32 minesNearby = 0;
};

static Cell g_cells[NUM_ROWS][NUM_COLS];


bool RandBool()
{
    if ((std::rand() % 2 + 1) == 1)
        return false;
    else
        return true;
}

void InitCells()
{
    uint32 mineCount = 0;
    for (uint32 r = 0; r < NUM_ROWS; r++)
    {
        for (uint32 c = 0; c < NUM_COLS; c++)
        {
            g_cells[r][c].isMine = false;
            g_cells[r][c].isRevealed = false;
            g_cells[r][c].isFlagged = false;
            g_cells[r][c].isGuessed = false;
            g_cells[r][c].isExploded = false;
            g_cells[r][c].isPressed = false;
            g_cells[r][c].minesNearby = 0;
            
            if (mineCount < NUM_MINES && RandBool() == 1)
            {
                g_cells[r][c].isMine = true;
                mineCount++;
            }
        }
    }
    
    for (uint32 r = 0; r < NUM_ROWS; r++)
    {
        for (uint32 c = 0; c < NUM_COLS; c++)
        {
            // left
            if (c > 0 && g_cells[r][c-1].isMine)
                g_cells[r][c].minesNearby++;
            
            // topleft
            if (r > 0 && c > 0 && g_cells[r-1][c-1].isMine)
                g_cells[r][c].minesNearby++;
            
            // top
            if (r > 0 && g_cells[r-1][c].isMine)
                g_cells[r][c].minesNearby++;
            
            // topright
            if (r > 0 && c < (NUM_COLS-1) && g_cells[r-1][c+1].isMine)
                g_cells[r][c].minesNearby++;
            
            // right
            if (c < (NUM_COLS-1) && g_cells[r][c+1].isMine)
                g_cells[r][c].minesNearby++;
            
            // bottomright
            if (r < (NUM_ROWS-1) && c < (NUM_COLS-1) && g_cells[r+1][c+1].isMine)
                g_cells[r][c].minesNearby++;
            
            // bottom
            if (r < (NUM_ROWS-1) && g_cells[r+1][c].isMine)
                g_cells[r][c].minesNearby++;
            
            // bottomleft
            if (r < (NUM_ROWS-1) && c > 0 && g_cells[r+1][c-1].isMine)
                g_cells[r][c].minesNearby++;
        }
    }
}

bool LoadImage(std::string file, SDL_Surface*& surface)
{
    std::string f = g_dataPath + file;
    const char* s = f.c_str();
    SDL_Surface* t = SDL_LoadBMP(s);
    if (!t)
    {
        ss.str("");
        ss << "Failed to load image: " << f;
        LOG_FAIL(ss.str());
        return false;
    }
    ss.str("");
    ss << "Loaded image: " << f;
    LOG_INFO(ss.str())
    surface = t;
    
    surface = SDL_ConvertSurface(t, g_screenSurface->format, 0);
    if (!surface)
    {
        SDL_FreeSurface(t); t = nullptr;
        ss.str("");
        ss << "Failed to optimize image: " << SDL_GetError();
        LOG_FAIL(ss.str());
        return false;
    }
    SDL_FreeSurface(t); t = nullptr;
    LOG_INFO("Optimized image.");
    
    return true;
}

bool AppInit()
{
    if (!VerifySingleInstanceInit())
        return false;
    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        ss.str("");
        ss << "Failed to initialize SDL: " << SDL_GetError();
        LOG_FAIL(ss.str());
        return false;
    }
    LOG_INFO("Initalized SDL.");
    
    g_window = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!g_window)
    {
        ss.str("");
        ss << "Failed to create window: " << SDL_GetError();
        LOG_FAIL(ss.str());
        return false;
    }
    g_screenSurface = SDL_GetWindowSurface(g_window);
    LOG_INFO("Created window.");
    
    if (!LoadImage("0.bmp", g_0Surface))
        return false;
    if (!LoadImage("1.bmp", g_1Surface))
        return false;
    if (!LoadImage("2.bmp", g_2Surface))
        return false;
    if (!LoadImage("3.bmp", g_3Surface))
        return false;
    if (!LoadImage("4.bmp", g_4Surface))
        return false;
    if (!LoadImage("5.bmp", g_5Surface))
        return false;
    if (!LoadImage("6.bmp", g_6Surface))
        return false;
    if (!LoadImage("7.bmp", g_7Surface))
        return false;
    if (!LoadImage("8.bmp", g_8Surface))
        return false;
    if (!LoadImage("flag.bmp", g_flagSurface))
        return false;
    if (!LoadImage("guess.bmp", g_guessSurface))
        return false;
    if (!LoadImage("pressed.bmp", g_pressedSurface))
        return false;
    if (!LoadImage("raised.bmp", g_raisedSurface))
        return false;
    if (!LoadImage("mine.bmp", g_explodedSurface))
        return false;
    
    InitCells();
    LOG_INFO("Initialized board.");
    
    std::srand(std::time(nullptr));
    LOG_INFO("Seeded random number generator.");
    
    return true;
}

void AppCleanup()
{
    LOG_INFO("Cleaning up.");
    
    if (g_raisedSurface)
        SDL_FreeSurface(g_raisedSurface); g_raisedSurface = nullptr;
    if (g_pressedSurface)
        SDL_FreeSurface(g_pressedSurface); g_pressedSurface = nullptr;
    if (g_guessSurface)
        SDL_FreeSurface(g_guessSurface); g_guessSurface = nullptr;
    if (g_flagSurface)
        SDL_FreeSurface(g_flagSurface); g_flagSurface = nullptr;
    if (g_8Surface)
        SDL_FreeSurface(g_8Surface); g_8Surface = nullptr;
    if (g_7Surface)
        SDL_FreeSurface(g_7Surface); g_7Surface = nullptr;
    if (g_6Surface)
        SDL_FreeSurface(g_6Surface); g_6Surface = nullptr;
    if (g_5Surface)
        SDL_FreeSurface(g_5Surface); g_5Surface = nullptr;
    if (g_4Surface)
        SDL_FreeSurface(g_4Surface); g_4Surface = nullptr;
    if (g_3Surface)
        SDL_FreeSurface(g_3Surface); g_3Surface = nullptr;
    if (g_2Surface)
        SDL_FreeSurface(g_2Surface); g_2Surface = nullptr;
    if (g_1Surface)
        SDL_FreeSurface(g_1Surface); g_1Surface = nullptr;
    if (g_0Surface)
        SDL_FreeSurface(g_0Surface); g_0Surface = nullptr;
    
    if (g_window)
        SDL_DestroyWindow(g_window); g_window = nullptr;
    
    SDL_Quit();
    VerifySingleInstanceCleanup();
    
    LOG_INFO("Exiting.");
}

bool DrawCell(uint32 row, uint32 col)
{
    SDL_Rect rect;
    rect.w = IMAGE_WIDTH;
    rect.h = IMAGE_HEIGHT;
    rect.x = IMAGE_WIDTH*col;
    rect.y = IMAGE_HEIGHT*row;
    Cell& c = g_cells[row][col];
    SDL_Surface* s = nullptr;
    if (c.isExploded)
    {
        s = g_explodedSurface;
    }
    else if (c.isRevealed)
    {
        switch (c.minesNearby)
        {
            case 0:
                s = g_0Surface;
                break;
            case 1:
                s = g_1Surface;
                break;
            case 2:
                s = g_2Surface;
                break;
            case 3:
                s = g_3Surface;
                break;
            case 4:
                s = g_4Surface;
                break;
            case 5:
                s = g_5Surface;
                break;
            case 6:
                s = g_6Surface;
                break;
            case 7:
                s = g_7Surface;
                break;
            case 8:
                s = g_8Surface;
                break;
            default:
                LOG_FAIL("Mines nearby exceeded 8 somehow.");
                return false;
                break;
        }
    }
    else if (c.isFlagged)
    {
        s = g_flagSurface;
    }
    else if (c.isGuessed)
    {
        s = g_guessSurface;
    }
    else if (c.isPressed)
    {
        s = g_pressedSurface;
    }
    else
    {
        s = g_raisedSurface;
    }
    
    if (SDL_BlitSurface(s, nullptr, g_screenSurface, &rect) != 0)
    {
        ss.str("");
        ss << "Failed to blit surface: " << SDL_GetError();
        LOG_FAIL(ss.str());
        return false;
    }
    
    return true;
}

bool DrawCells()
{
    for (uint32 r = 0; r < NUM_ROWS; r++)
    {
        for (uint32 c = 0; c < NUM_COLS; c++)
        {
            if (!DrawCell(r, c))
                return false;
        }
    }
    
    return true;
}

int AppLoop()
{
    bool quit = false;
    SDL_Event e;
    while (!quit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                quit = true;
        }
        
        if (!DrawCells())
            return -1;
        
        if (SDL_UpdateWindowSurface(g_window) != 0)
        {
            ss.str("");
            ss << "Failed to update window surface: " << SDL_GetError();
            LOG_FAIL(ss.str());
            return -1;
        }
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
