# All Taps -- Guitar Hero World Tour: Definitive Edition

Patches Guitar Hero World Tour: Definitive Edition so every note is a tap note. No strumming required -- press frets to hit notes.

---

## Requirements

- Guitar Hero World Tour: Definitive Edition (GHWTDE) installed and launchable
- Windows (32-bit or 64-bit)

---

## Install

1. Download `all-taps-ghwt-X.Y.Z-win32.zip` from the [Releases page](../../releases).
2. Open the ZIP and copy `xinput1_3.dll` into the GHWTDE game folder (the same folder as `GHWT_Definitive.exe`).
3. Launch the game. All notes behave as taps immediately -- no configuration needed.

Typical Steam install path:

```
C:\Program Files (x86)\Steam\steamapps\common\Guitar Hero World Tour Definitive Edition
```

---

## Verify

1. Start any song on any difficulty.
2. Press fret keys or fret buttons without strumming.
3. Every note should register on fret press alone.

---

## Uninstall

1. Delete `xinput1_3.dll` from the GHWTDE game folder.
2. Launch the game. Original strum behavior is restored immediately.

---

## Antivirus / SmartScreen

Windows may show a SmartScreen warning or your antivirus may flag `xinput1_3.dll`. This is expected behavior for unsigned community mods -- the DLL is not code-signed.

Scan results: [VirusTotal scan (vX.Y.Z)](link-to-results) -- X/72 detections, all heuristic false positives common for xinput proxy DLLs.

To bypass the warning, use either method:

**Method 1 -- Unblock via Properties:**
1. Right-click `xinput1_3.dll` in Explorer.
2. Select Properties.
3. At the bottom of the General tab, check "Unblock" and click OK.

**Method 2 -- SmartScreen bypass:**
1. When SmartScreen appears, click "More info".
2. Click "Run anyway".

---

## How It Works

`xinput1_3.dll` is loaded as a proxy. Windows resolves it before the system copy because it sits in the game folder (DLL search order). The proxy forwards all XInput calls to the real system `xinput1_3.dll` and then patches three functions in `GHWTDE.dll` at runtime:

- `CanAutoStrum` -- patched to always return `true`
- `b_AutoStrumAllowed` -- set to `true`
- `is_tapping_note` -- patched to always return `true`

No fake input is injected. The game's own logic natively treats every note as a tap note. This works transparently with keyboard, gamepad, and any other input method.

---

## Building from Source

**Prerequisites:**
- CMake 3.25 or newer
- MSVC (Visual Studio Build Tools, with Desktop development with C++ workload)
- Windows (32-bit target required)

**Commands:**

```bat
cmake --preset win32-release
cmake --build --preset win32-release --config Release
```

**Output:** `build/win32-release/Release/xinput1_3.dll`

---

## License

MIT License

Copyright (c) 2026 all-taps-ghwt contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Source: https://github.com/[your-username]/all-taps-ghwt
