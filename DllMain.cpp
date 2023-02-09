#include <Windows.h>
#include <mhook.h>
#include <cstdio>
#include <cstdint>

typedef HMODULE(WINAPI *PFNLOADLIBRARYA)(LPCSTR);
PFNLOADLIBRARYA OrigLoadLibraryA = &LoadLibraryA;

typedef bool(__cdecl *PFNDllSlashedInternalApplyConfiguration)(char *config);
PFNDllSlashedInternalApplyConfiguration DllSlashedInternalApplyConfiguration;

bool g_isFullscreen = false;

HMODULE g_slash1Mod = nullptr;
unsigned char *g_slash1ModBytes = nullptr;

bool __cdecl HookedDllSlashedInternalApplyConfiguration(char *config) {
    // fix up the config

    // g_isFullscreen = config[0x14];

    config[0x14] = 0;

    int cxScreen = GetSystemMetrics(SM_CXSCREEN);
    int cyScreen = GetSystemMetrics(SM_CYSCREEN);

    uint32_t width = *reinterpret_cast<uint32_t *>(config + 0xC);
    uint32_t height = *reinterpret_cast<uint32_t *>(config + 0x10);

    g_isFullscreen = width == cxScreen && height == cyScreen;

    uint32_t *pWndStyle = reinterpret_cast<uint32_t *>(g_slash1ModBytes + 0x1B5D);
    if (g_isFullscreen) {
        *pWndStyle = WS_POPUP | WS_MINIMIZEBOX;
    } else {
        *pWndStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    return DllSlashedInternalApplyConfiguration(config);
}

HMODULE WINAPI HookedLoadLibraryA(LPCSTR libName) {
    HMODULE hmod = OrigLoadLibraryA(libName);
    if (!g_slash1Mod && hmod && strcmp(libName, "slash1.dll") == 0) {
        g_slash1Mod = OrigLoadLibraryA(libName); // force it to never unload, increasing refcount

        DllSlashedInternalApplyConfiguration = (PFNDllSlashedInternalApplyConfiguration)
            GetProcAddress(hmod, "DllSlashedInternalApplyConfiguration");

        char buf[1024];
        sprintf(buf, "DllSlashedInternalApplyConfiguration: base + %#08x\n",
            ((char *) DllSlashedInternalApplyConfiguration) - ((char *) hmod));
        OutputDebugStringA(buf);

        Mhook_SetHook((void **)&DllSlashedInternalApplyConfiguration,
            (void *) &HookedDllSlashedInternalApplyConfiguration);

        g_slash1ModBytes = (unsigned char *) hmod;

        DWORD oldProtect;
        VirtualProtect(g_slash1ModBytes, 0x2000, PAGE_EXECUTE_READWRITE, &oldProtect);

        g_slash1ModBytes[0x1B56] = 0xEB; // patch out the fullscreen window style code
    }
    return hmod;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        Mhook_SetHook((void **)&OrigLoadLibraryA,
            (void *) &HookedLoadLibraryA);
    }
    return TRUE;
}