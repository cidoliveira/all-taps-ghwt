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
// All-Taps GHWTDE patch
// ---------------------------------------------------------------------------
//
// GHWTDE.dll exports all symbols (GCC/MinGW Itanium ABI mangling).
// We patch two functions and one flag to make all notes taps:
//
//   1. CanAutoStrum()      — stubbed to return false; we patch to return true
//   2. b_AutoStrumAllowed  — global bool; we set to true
//   3. is_tapping_note()   — per-note check; we patch to always return true
//
// Total: 8 bytes of code + 1 byte of data. No MinHook needed.
//

static bool g_ghwtdePatched = false;

static void TryPatchGHWTDE()
{
    if (g_ghwtdePatched) return;

    HMODULE ghwtde = GetModuleHandleA("GHWTDE.dll");
    if (!ghwtde) return;  // Not loaded yet, try next frame

    g_ghwtdePatched = true;
    OutputDebugStringA("[AllTaps] GHWTDE.dll found, applying patches...\n");

    // Tier 1: Patch CanAutoStrum() -> return true  (was: xor eax,eax; ret)
    auto canAutoStrum = reinterpret_cast<BYTE*>(
        GetProcAddress(ghwtde, "_ZN9InputHook12CanAutoStrumEv"));
    if (canAutoStrum) {
        DWORD oldProt;
        if (VirtualProtect(canAutoStrum, 3, PAGE_EXECUTE_READWRITE, &oldProt)) {
            canAutoStrum[0] = 0xB0;  // mov al, 1
            canAutoStrum[1] = 0x01;
            canAutoStrum[2] = 0xC3;  // ret
            VirtualProtect(canAutoStrum, 3, oldProt, &oldProt);
            OutputDebugStringA("[AllTaps] Patched CanAutoStrum -> return true\n");
        }
    }

    // Tier 1: Set b_AutoStrumAllowed = true
    auto bAutoStrum = reinterpret_cast<bool*>(
        GetProcAddress(ghwtde, "_ZN9InputHook18b_AutoStrumAllowedE"));
    if (bAutoStrum) {
        *bAutoStrum = true;
        OutputDebugStringA("[AllTaps] Set b_AutoStrumAllowed = true\n");
    }

    // Tier 2: Patch is_tapping_note(int,int) -> return true
    //   Convention: __thiscall (ecx=this), 2 int params, ret 8
    auto isTapping = reinterpret_cast<BYTE*>(
        GetProcAddress(ghwtde, "_ZN3Mdl5Input15is_tapping_noteEii"));
    if (isTapping) {
        DWORD oldProt;
        if (VirtualProtect(isTapping, 5, PAGE_EXECUTE_READWRITE, &oldProt)) {
            isTapping[0] = 0xB0;  // mov al, 1
            isTapping[1] = 0x01;
            isTapping[2] = 0xC2;  // ret 8
            isTapping[3] = 0x08;
            isTapping[4] = 0x00;
            VirtualProtect(isTapping, 5, oldProt, &oldProt);
            OutputDebugStringA("[AllTaps] Patched is_tapping_note -> return true\n");
        }
    }

    // Log what resolved and what didn't
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[AllTaps] Patch summary: CanAutoStrum=%s, b_AutoStrum=%s, is_tapping=%s\n",
             canAutoStrum ? "OK" : "MISSING",
             bAutoStrum   ? "OK" : "MISSING",
             isTapping    ? "OK" : "MISSING");
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
    TryPatchGHWTDE();  // lazy: no-op after first successful patch
    auto fn = reinterpret_cast<PFN_XInputGetState>(g_procs[2]);
    if (!fn) return ERROR_DEVICE_NOT_CONNECTED;

    DWORD result = fn(dwUserIndex, pState);

    if (result == ERROR_SUCCESS && pState != nullptr && dwUserIndex < XUSER_MAX_COUNT)
    {
        // SAFE-02: Guitar controllers have a physical strum bar.
        // Skip fret detection entirely for guitar slots.
        if (g_isGuitar[dwUserIndex])
            return result;

        WORD fretBits = pState->Gamepad.wButtons & FRET_MASK;
        if (fretBits != 0)
        {
            DebugLogFrets(dwUserIndex, fretBits);
        }
    }

    return result;
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
