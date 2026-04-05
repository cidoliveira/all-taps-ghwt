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

Scan results: [VirusTotal scan (v1.0.0)](link-to-results)

Detections are heuristic false positives common for xinput proxy DLLs. Antivirus engines like Cynet are known to flag virtually anything unsigned -- do not count those as real detections.

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

MIT -- see [LICENSE](LICENSE).
