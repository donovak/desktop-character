# Desktop Character

Native Windows desktop character prototype built with C++20, Win32, Direct2D, and CMake.

## Current Phase

Phase 2 is focused on safe desktop-overlay behavior:

- `DesktopOverlay` is the default mode.
- `NormalWindow` preserves the Phase 1 overlapped test window for debugging.
- The overlay uses virtual desktop metrics from `SM_XVIRTUALSCREEN`, `SM_YVIRTUALSCREEN`, `SM_CXVIRTUALSCREEN`, and `SM_CYVIRTUALSCREEN`.
- The overlay is borderless and shown without activation where possible.
- The overlay can use simple color-key transparency. This is not per-pixel alpha and is intentionally easy to remove later.
- Click-through is disabled by default.

Out of scope for this phase: WorkerW, Shell COM, desktop icon detection, wallpapers, weapons, and icon collision.

## Switching Window Modes

The app defaults to desktop overlay mode:

```powershell
.\build\Debug\DesktopCharacter.exe
```

Run as the original standard test window:

```powershell
.\build\Debug\DesktopCharacter.exe --normal-window
```

Force overlay mode explicitly:

```powershell
.\build\Debug\DesktopCharacter.exe --desktop-overlay
```

Useful overlay flags:

```powershell
.\build\Debug\DesktopCharacter.exe --desktop-overlay --opaque-overlay
.\build\Debug\DesktopCharacter.exe --desktop-overlay --enable-click-through
```

`--enable-click-through` is a clearly named debug flag and is off by default. Press `Esc` to close the prototype.

## Build From VSCode CMake Tools

1. Open this folder in VSCode.
2. Run `CMake: Select Kit` and choose an MSVC Visual Studio kit.
3. Run `CMake: Configure`.
4. Run `CMake: Build`.
5. Run `build\Debug\DesktopCharacter.exe` directly or launch the `DesktopCharacter` target from CMake Tools.

## Command-Line Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\build\Debug\DesktopCharacter.exe
```
