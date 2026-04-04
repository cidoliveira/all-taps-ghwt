#include "xinput_proxy.h"

#include <windows.h>
#include <xinput.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

HMODULE g_realXInput = nullptr;
FARPROC g_procs[9]   = {};

// Per-slot device classification
bool g_isGuitar[XUSER_MAX_COUNT]      = {};
bool g_slotConnected[XUSER_MAX_COUNT] = {};

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
// Guitar subtype detection
// ---------------------------------------------------------------------------

// Older SDK versions may omit GUITAR_ALTERNATE and GUITAR_BASS.
#ifndef XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE
#define XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE 0x07
#endif
#ifndef XINPUT_DEVSUBTYPE_GUITAR_BASS
#define XINPUT_DEVSUBTYPE_GUITAR_BASS      0x0B
#endif

// Returns true if subType identifies a guitar controller (has physical strum bar).
// Three XInput guitar subtypes exist: GUITAR (0x06), GUITAR_ALTERNATE (0x07),
// GUITAR_BASS (0x0B). All other subtypes (gamepad, wheel, dance pad, etc.) return false.
inline bool IsGuitarSubtype(BYTE subType)
{
    return subType == XINPUT_DEVSUBTYPE_GUITAR
        || subType == XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE
        || subType == XINPUT_DEVSUBTYPE_GUITAR_BASS;
}

// ---------------------------------------------------------------------------
// Fret detection
// ---------------------------------------------------------------------------

// Combined fret button bitmask.
// Guitar Hero maps fret buttons to standard XInput gamepad buttons:
//   Green  = A  (0x1000)    Red    = B  (0x2000)
//   Yellow = Y  (0x8000)    Blue   = X  (0x4000)
//   Orange = LB (0x0100)
static constexpr WORD FRET_MASK =
    XINPUT_GAMEPAD_A             |  // Green  0x1000
    XINPUT_GAMEPAD_B             |  // Red    0x2000
    XINPUT_GAMEPAD_Y             |  // Yellow 0x8000
    XINPUT_GAMEPAD_X             |  // Blue   0x4000
    XINPUT_GAMEPAD_LEFT_SHOULDER;   // Orange 0x0100
// = 0xF100

// Log fret press to OutputDebugString. Viewable in Sysinternals DebugView.
// Only called when fret bits are non-zero, so no per-frame spam.
inline void DebugLogFrets(DWORD slot, WORD fretBits)
{
    char buf[80];
    sprintf_s(buf, sizeof(buf),
              "[AllTaps] slot=%lu frets=0x%04X\n",
              slot, (unsigned)fretBits);
    OutputDebugStringA(buf);
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
DWORD WINAPI XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags,
                                   XINPUT_CAPABILITIES* pCapabilities) WIN_NOEXCEPT
{
    static bool s_init = (LoadRealXInput(), true);
    auto fn = reinterpret_cast<PFN_XInputGetCapabilities>(g_procs[4]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;

    DWORD result = fn(dwUserIndex, dwFlags, pCapabilities);

    // Cache device subtype per slot for use by XInputGetState
    if (dwUserIndex < XUSER_MAX_COUNT)
    {
        if (result == ERROR_SUCCESS && pCapabilities != nullptr)
        {
            g_isGuitar[dwUserIndex]      = IsGuitarSubtype(pCapabilities->SubType);
            g_slotConnected[dwUserIndex] = true;

            // Diagnostic: log first capabilities query per slot (one-shot)
            static bool s_logged[XUSER_MAX_COUNT] = {};
            if (!s_logged[dwUserIndex])
            {
                char buf[128];
                sprintf_s(buf, sizeof(buf),
                          "[AllTaps] GetCapabilities slot=%lu subType=0x%02X flags=0x%08lX guitar=%s\n",
                          dwUserIndex,
                          (unsigned)pCapabilities->SubType,
                          dwFlags,
                          g_isGuitar[dwUserIndex] ? "YES" : "NO");
                OutputDebugStringA(buf);
                s_logged[dwUserIndex] = true;
            }
        }
        else
        {
            // Disconnected or error: clear cached state so reconnects
            // with a different device type are handled correctly.
            g_isGuitar[dwUserIndex]      = false;
            g_slotConnected[dwUserIndex] = false;
        }
    }

    return result;
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
