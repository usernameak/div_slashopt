#include <Windows.h>
#include <mhook.h>
#include <cstdio>
#include <cstdint>

// === COMMON === //

bool g_isFullscreen = false;

// === BINK VIDEO HOOKS === //

HMODULE g_binkMod               = nullptr;

typedef int32_t(__stdcall *BINKCOPYTOBUFFER)(void *bnk, void *dest, int32_t destpitch, uint32_t destheight, uint32_t destx, uint32_t desty, uint32_t flags);
BINKCOPYTOBUFFER BinkCopyToBuffer = nullptr;

// change Bink output from RGB565 to RGBA8888
int32_t __stdcall Hooked_BinkCopyToBuffer(
    void *bnk,
    void *dest,
    int32_t destpitch,
    uint32_t destheight,
    uint32_t destx,
    uint32_t desty,
    uint32_t flags) {

    return BinkCopyToBuffer(bnk, dest, destpitch, destheight, destx, desty, (flags & ~0xF) | 3);
}

// === SLASHED RENDERER HOOKS === //

HMODULE g_slash1Mod             = nullptr;
unsigned char *g_slash1ModBytes = nullptr;

typedef bool(__cdecl *PFNDllSlashedInternalApplyConfiguration)(char *config);
PFNDllSlashedInternalApplyConfiguration DllSlashedInternalApplyConfiguration = nullptr;

bool __cdecl HookedDllSlashedInternalApplyConfiguration(char *config) {
    config[0x14] = 0; // force windowed mode for the engine

    // get native primary monitor resolution
    int cxScreen = GetSystemMetrics(SM_CXSCREEN);
    int cyScreen = GetSystemMetrics(SM_CYSCREEN);

    // get the display width/height from the config
    uint32_t width  = *reinterpret_cast<uint32_t *>(config + 0xC);
    uint32_t height = *reinterpret_cast<uint32_t *>(config + 0x10);

    // is the resolution fullscreen?
    g_isFullscreen = width == cxScreen && height == cyScreen;

    uint32_t *pWndStyle = reinterpret_cast<uint32_t *>(g_slash1ModBytes + 0x1B5D);
    if (g_isFullscreen) {
        // patch the window style to borderless fullscreen
        *pWndStyle = WS_POPUP | WS_MINIMIZEBOX;
    } else {
        // patch the window style to a normal window (non-resizable)
        *pWndStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    return DllSlashedInternalApplyConfiguration(config);
}

// === LOADER HOOKS === //

typedef HMODULE(WINAPI *PFNLOADLIBRARYA)(LPCSTR);
PFNLOADLIBRARYA OrigLoadLibraryA = &LoadLibraryA;

HMODULE WINAPI HookedLoadLibraryA(LPCSTR libName) {
    HMODULE hmod = OrigLoadLibraryA(libName);

    // if we are loading slash1.dll (D3D6 renderer library) for the first time
    if (!g_slash1Mod && hmod && strcmp(libName, "slash1.dll") == 0) {
        g_slash1Mod = OrigLoadLibraryA(libName); // force it to never unload, increasing refcount
        g_slash1ModBytes = (unsigned char *)hmod;

        // hook Slashed rendering engine initialization
        DllSlashedInternalApplyConfiguration = (PFNDllSlashedInternalApplyConfiguration)
            GetProcAddress(hmod, "DllSlashedInternalApplyConfiguration");

#ifdef _DEBUG
        char buf[1024];
        sprintf(buf, "DllSlashedInternalApplyConfiguration: base + %#08x\n",
            ((char *)DllSlashedInternalApplyConfiguration) - ((char *)hmod));
        OutputDebugStringA(buf);
#endif

        Mhook_SetHook((void **)&DllSlashedInternalApplyConfiguration,
            (void *)&HookedDllSlashedInternalApplyConfiguration);

        // patch out the fullscreen window style code, we handle that ourselves.
        DWORD oldProtect;
        VirtualProtect(g_slash1ModBytes, 0x2000, PAGE_EXECUTE_READWRITE, &oldProtect);
        g_slash1ModBytes[0x1B56] = 0xEB;

        // hook Bink
        g_binkMod   = OrigLoadLibraryA("binkw32.dll");
        BinkCopyToBuffer = (BINKCOPYTOBUFFER)GetProcAddress(g_binkMod, "_BinkCopyToBuffer@28");
        Mhook_SetHook((void **)&BinkCopyToBuffer, (void *)&Hooked_BinkCopyToBuffer);
    }
    return hmod;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        Mhook_SetHook((void **)&OrigLoadLibraryA,
            (void *)&HookedLoadLibraryA);

        DWORD oldProtect;
        VirtualProtect((void *)(uintptr_t)0x48D000, 0x1000, PAGE_EXECUTE_READWRITE, &oldProtect);

        // default `resolution switch` in config.lcl to 0 rather than 1
        *((unsigned char *)0x48D9AF) = 0x9E;
    }

    return TRUE;
}