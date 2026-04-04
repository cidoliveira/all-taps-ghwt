#include "xinput_proxy.h"

#include <windows.h>
#include <xinput.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

HMODULE g_realXInput = nullptr;
FARPROC g_procs[9]   = {};

// ---------------------------------------------------------------------------
// LoadRealXInput
// ---------------------------------------------------------------------------

bool LoadRealXInput()
{
    if (g_realXInput != nullptr)
        return true;

    char sysdir[MAX_PATH];
    GetSystemDirectoryA(sysdir, MAX_PATH);

    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s\\xinput1_3.dll", sysdir);

    g_realXInput = LoadLibraryA(path);
    if (g_realXInput == nullptr)
        return false;

    // Ordinals 2-8 map to the 7 XInput exports
    for (int i = 2; i <= 8; ++i)
        g_procs[i] = GetProcAddress(g_realXInput, (LPCSTR)(ULONG_PTR)i);

    return true;
}

// ---------------------------------------------------------------------------
// Export stubs
// WIN_NOEXCEPT is required to match the exception specification in xinput.h.
// Without it, MSVC raises C2382 (redefinition with different exception spec).
// ---------------------------------------------------------------------------

typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, XINPUT_STATE*);
DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputGetState>(g_procs[2]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;
    return fn(dwUserIndex, pState);
}

typedef DWORD (WINAPI *PFN_XInputSetState)(DWORD, XINPUT_VIBRATION*);
DWORD WINAPI XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputSetState>(g_procs[3]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;
    return fn(dwUserIndex, pVibration);
}

typedef DWORD (WINAPI *PFN_XInputGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
DWORD WINAPI XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputGetCapabilities>(g_procs[4]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;
    return fn(dwUserIndex, dwFlags, pCapabilities);
}

typedef void (WINAPI *PFN_XInputEnable)(BOOL);
void WINAPI XInputEnable(BOOL enable) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputEnable>(g_procs[5]);
    if (!fn) return;
    fn(enable);
}

typedef DWORD (WINAPI *PFN_XInputGetDSoundAudioDeviceGuids)(DWORD, GUID*, GUID*);
DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputGetDSoundAudioDeviceGuids>(g_procs[6]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;
    return fn(dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid);
}

typedef DWORD (WINAPI *PFN_XInputGetBatteryInformation)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);
DWORD WINAPI XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* pBatteryInformation) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputGetBatteryInformation>(g_procs[7]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;
    return fn(dwUserIndex, devType, pBatteryInformation);
}

typedef DWORD (WINAPI *PFN_XInputGetKeystroke)(DWORD, DWORD, PXINPUT_KEYSTROKE);
DWORD WINAPI XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputGetKeystroke>(g_procs[8]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;
    return fn(dwUserIndex, dwReserved, pKeystroke);
}
