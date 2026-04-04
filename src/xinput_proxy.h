#pragma once

#include <windows.h>
#include <xinput.h>

// Handle to the real system xinput1_3.dll loaded from System32/SysWOW64
extern HMODULE g_realXInput;

// Proc address array indexed by ordinal.
// Ordinals 2-8 are the XInput exports (slots 0 and 1 are unused).
extern FARPROC g_procs[9];

// Load the real xinput1_3.dll from the system directory.
// Idempotent: safe to call multiple times.
bool LoadRealXInput();
