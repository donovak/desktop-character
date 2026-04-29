# Desktop Character

Native Windows desktop character prototype built with C++20, Win32, Direct2D, and CMake.

## Current Phase

Phase 3 is focused on read-only desktop icon awareness:

- `DesktopOverlay` is the default mode.
- `NormalWindow` preserves the Phase 1 overlapped test window for debugging.
- The overlay uses virtual desktop metrics from `SM_XVIRTUALSCREEN`, `SM_YVIRTUALSCREEN`, `SM_CXVIRTUALSCREEN`, and `SM_CYVIRTUALSCREEN`.
- The overlay is borderless and shown without activation where possible.
- The overlay can use simple color-key transparency. This is not per-pixel alpha and is intentionally easy to remove later.
- Click-through is disabled by default.
- Desktop icons are discovered through Shell COM APIs and drawn as debug rectangles.
- Icon discovery is read-only.

Out of scope for this phase: opening icons, selecting icons, moving icons, icon collision, WorkerW, wallpapers, weapons, and advanced physics.

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

## Desktop Icon Debug Overlay

The icon debug overlay is on by default.

- Press `F2` to toggle icon rectangles and labels.
- Press `F5` to refresh the desktop icon cache.
- Press `Esc` to close the prototype.

Icon rectangles are mapped from desktop Shell view coordinates into the app client area by converting the Shell view icon position to screen coordinates, then subtracting the app window client origin. This is intended primarily for `DesktopOverlay`; in `NormalWindow`, desktop icon rectangles may be outside or offset from the small debug window.

Current limitations:

- Bounds are estimated from the desktop icon position plus system icon spacing.
- Labels use a simple DirectWrite debug rendering path.
- The cache refresh is manual/startup only; full shell change notifications are left for a later phase.
- Some virtual or special desktop items may not have filesystem paths.

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
