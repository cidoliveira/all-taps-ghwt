# All Taps GHWT

## What This Is

An auto-strum mod for Guitar Hero World Tour Definitive Edition (GHWTDE) that eliminates the need to press a strum key/button when playing with keyboard or gamepad. When a player presses any fret button, the mod automatically injects the strum signal so notes register instantly. Packaged for the GHWTDE community as a drop-in install.

## Core Value

Fret press = note hit. No strum required. Keyboard and gamepad players get a natural input experience without the guitar controller legacy mechanic.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Auto-strum triggers on any fret button press (Green, Red, Yellow, Blue, Orange)
- [ ] Works with keyboard input
- [ ] Works with XInput gamepad input
- [ ] Works with DirectInput/SDL gamepad input (PS4 controllers etc.)
- [ ] Integrates with GHWTDE mod loading (DLL-based)
- [ ] Always-on behavior, no toggle needed
- [ ] Does not interfere with other game inputs (menus, star power, whammy)
- [ ] Compatible with GHWTDE v1.4.3.4+
- [ ] Community-ready packaging (README, install instructions)
- [ ] Chord support (multiple frets pressed simultaneously still trigger one strum)

### Out of Scope

- Guitar controller support — guitar controllers already have a strum bar, this mod solves keyboard/gamepad only
- Per-song toggle or configuration UI — always on, zero config
- Online/multiplayer anti-cheat considerations — this is a singleplayer mod
- Modifying song chart data — we hook input, not song files

## Context

- GHWTDE is installed at `C:\Users\alcid\Desktop\Jogos\GHWT`
- Game config lives at `C:\Users\alcid\Documents\My Games\Guitar Hero World Tour Definitive Edition\`
- GHWTDE.dll (2.5MB) is the Definitive Edition's core mod — the game loads it alongside GHWT_Definitive.exe
- The MODS folder (`DATA/MODS/`) supports songs and characters; gameplay mods use DLL injection
- Game scripts are encrypted .qb.xen files — not modifiable without decompilation tools
- Input config (GHWTDEInput.ini) maps controller buttons to game actions via XInput device sections
- Keyboard fret mappings in GHWTDE.ini: Green=4, Red=22, Yellow=13, Blue=14, Orange=15 (Keyboard_Menu section)
- Strum on guitar controllers maps to DPad Up/Down — keyboard strum key needs to be discovered
- Game version: 1.4.3.4 (March 29, 2026)

## Constraints

- **Compatibility**: Must not conflict with GHWTDE.dll — load alongside it, not replace it
- **Input latency**: Strum injection must be near-zero latency to not affect gameplay feel
- **Platform**: Windows only (GHWTDE is Windows-only)
- **Language**: C or C++ for DLL mod (game is native Win32)
- **Distribution**: Must be easy to install — ideally drop a DLL into the game folder

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| DLL hook approach over external tool | Integrates with game input pipeline directly, works across all input devices, lower latency than input simulation | — Pending |
| Always-on, no toggle | Simplicity for the target audience (keyboard/gamepad players who never want to strum) | — Pending |
| Exclude guitar controllers | Guitar players already have strum bar — mod solves a keyboard/gamepad problem | — Pending |

---
*Last updated: 2026-04-04 after initialization*
