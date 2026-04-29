# PLAN.md

## Vision

Build a native Windows desktop character app in C++20.

The character should feel like it naturally lives on the Windows desktop:
- smooth movement
- desktop-aware positioning
- icon interactions
- optional interactive wallpapers/backgrounds
- playful actions like hitting, shooting, or triggering desktop icons

The long-term goal is a polished desktop playground layered over the real Windows desktop.

---

## Phase 1 — Native Window + Character Prototype

Goal: prove the basic native app loop.

Deliverables:
- C++20 + CMake project
- Win32 desktop window
- Direct2D rendering
- simple placeholder character
- WASD / arrow-key movement
- frame-rate independent updates
- clean modular structure

Out of scope:
- desktop icons
- collisions
- Shell APIs
- WorkerW
- wallpapers
- weapons

Exit criteria:
- app builds and runs
- character moves smoothly
- codebase is clean enough to extend

---

## Phase 2 — Desktop Overlay Behavior

Goal: make the app feel like it belongs on the desktop.

Deliverables:
- borderless transparent window
- virtual desktop sizing
- basic multi-monitor awareness
- optional click-through mode
- no annoying focus stealing
- debug overlay toggle

Exit criteria:
- desktop remains usable
- character appears naturally over/near the desktop
- window behavior is stable

---

## Phase 3 — Desktop Icon Awareness

Goal: discover real desktop icons and their positions.

Preferred approach:
- Shell COM APIs
- `IShellWindows`
- `IShellBrowser`
- `IShellView`
- `IFolderView`
- `IShellFolder`

Deliverables:
- enumerate desktop items
- read icon names
- read icon positions
- render debug boxes around icons
- refresh icon cache when needed

Fallbacks:
- `SysListView32` messages for diagnostics
- UI Automation only if needed

Exit criteria:
- app can accurately map visible desktop icons to screen-space rectangles

---

## Phase 4 — Basic Icon Interaction

Goal: allow the character to interact with icons.

Deliverables:
- collision detection with icon rectangles
- interact key
- basic “hit” animation
- open icon via `ShellExecuteEx` or shell verb path
- simple cooldown to avoid repeated accidental opens

Interactions:
- walk into icon + press key
- attack icon + open it
- optional hover/selection highlight

Exit criteria:
- files, folders, and shortcuts open naturally

---

## Phase 5 — Character Polish

Goal: make the character feel alive.

Deliverables:
- sprite support
- idle/walk/attack animations
- directional facing
- sound effects
- configurable movement speed
- simple particle effects
- basic settings file

Exit criteria:
- placeholder shape is replaced with a character that feels responsive and polished

---

## Phase 6 — Interactive Wallpaper / Background System

Goal: support wallpapers designed specifically for the app.

Concept:
- the real Windows desktop remains available
- custom background data defines walkable areas, platforms, zones, and interactables
- the character can move through the desktop like a small game world

Deliverables:
- background definition format, likely JSON
- background image loading
- walkable regions
- blocked regions
- platforms / ledges
- interaction zones
- optional parallax or animated background elements

Example background data:
```json
{
  "image": "assets/backgrounds/room.png",
  "walkableAreas": [
    { "x": 0, "y": 850, "width": 1920, "height": 230 }
  ],
  "platforms": [
    { "x": 300, "y": 650, "width": 250, "height": 24 }
  ],
  "interactables": [
    {
      "name": "lamp",
      "x": 1200,
      "y": 760,
      "width": 80,
      "height": 120,
      "action": "toggle_light"
    }
  ]
}
```

Exit criteria:
- at least one custom wallpaper supports terrain-style movement and simple interactions

---

## Phase 7 — Advanced Movement and Physics

Goal: make desktop movement more game-like.

Deliverables:
- gravity
- jumping
- grounded state
- platform collision
- optional wall collision
- simple movement tuning
- optional controller support

Exit criteria:
- character can walk, jump, and land on designed wallpaper/platform geometry

---

## Phase 8 — Advanced Icon Interactions

Goal: make icon interaction playful.

Possible interactions:
- melee hit opens icon
- projectile hit opens icon
- drag/push-style reactions
- icon shake animation
- icon health bar before opening
- hold-to-select
- special actions for folders, apps, shortcuts, recycle bin

Deliverables:
- action system
- projectile system
- hit detection
- visual effects
- configurable interaction mode

Exit criteria:
- icons can be interacted with in multiple satisfying ways

---

## Phase 9 — Desktop Reliability Hardening

Goal: handle real Windows weirdness.

Deliverables:
- Explorer restart recovery
- DPI handling
- monitor layout changes
- icon refresh when desktop changes
- settings persistence
- crash logging
- safe fallback modes

Exit criteria:
- app survives normal desktop changes without needing manual restart

---

## Phase 10 — Optional WorkerW / Native Desktop Embedding

Goal: experiment with making visuals appear more deeply embedded in the desktop layer.

Important:
- this relies on undocumented Explorer behavior
- it must be isolated behind a feature flag
- the app must work without it

Deliverables:
- optional WorkerW host mode
- fallback to overlay mode
- Windows version checks
- diagnostics for failed attachment

Exit criteria:
- experimental mode improves visual integration but never breaks the main app

---

## Phase 11 — Packaging and Showcase

Goal: make the project presentable.

Deliverables:
- release build
- README
- demo video/GIF
- settings guide
- architecture writeup
- known limitations

---

## Current Phase

Current phase: **Phase 1**

