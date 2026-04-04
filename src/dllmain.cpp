#include "xinput_proxy.h"

// Minimal DllMain.
// - No LoadLibrary or heavy work on attach (avoids loader lock deadlock).
// - Lazy loading is handled by the s_init pattern in each export stub.
// - FreeLibrary on detach to release the real DLL handle cleanly.
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID /*reserved*/)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        break;

    case DLL_PROCESS_DETACH:
        if (g_realXInput != nullptr)
        {
            FreeLibrary(g_realXInput);
            g_realXInput = nullptr;
        }
        break;

    default:
        break;
    }
    return TRUE;
}
