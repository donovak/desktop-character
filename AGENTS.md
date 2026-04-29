# AGENTS.md

## Project Overview

This project is a **native Windows desktop character application** written in **C++20**.

The goal is to create a small animated character that lives on the Windows desktop, renders smoothly, and can eventually interact with desktop icons and the Windows shell.

This is a **multi-phase project**. Agents MUST only implement the currently specified phase unless explicitly instructed otherwise.

---

## Core Principles

1. **Native First**

   * Use Win32 APIs directly where appropriate.
   * Avoid heavy frameworks (no Qt, no Electron, no .NET).
   * Prefer lightweight, low-level control.

2. **Incremental Development**

   * Build features in phases.
   * Do NOT implement future-phase features early.
   * Each step must compile and run independently.

3. **Clean Architecture**

   * Maintain clear separation of concerns.
   * Avoid monolithic files.
   * Prefer small, focused classes.

4. **Performance Matters**

   * Target smooth rendering (60 FPS baseline).
   * Avoid unnecessary allocations in render loops.
   * Prefer stack allocation and RAII.

5. **Safety and Stability**

   * Use RAII for all resource management.
   * Avoid raw `new` / `delete` unless absolutely necessary.
   * Prefer `std::unique_ptr`, `std::vector`, etc.

---

## Technology Stack

* Language: C++20
* Build System: CMake
* Compiler: MSVC (Visual Studio toolchain)
* Rendering: Direct2D (initial), potentially DirectComposition later
* Windowing: Win32 API

---

## Project Structure

```
/src
    main.cpp
    App.*
    DesktopWindow.*
    Renderer.*
    Input.*
    Character.*

/docs
    PLAN.md

AGENTS.md
CMakeLists.txt
```

### Responsibilities

* **App**

  * Entry point coordination
  * Initializes subsystems
  * Runs main loop

* **DesktopWindow**

  * Creates and manages the Win32 window
  * Handles window styles (borderless, transparent, click-through later)

* **Renderer**

  * Handles Direct2D initialization
  * Draws the character and future elements
  * Owns render loop timing

* **Input**

  * Handles keyboard input (initially simple polling)
  * Later: raw input

* **Character**

  * Position, velocity, movement logic
  * Future: animation state

---

## Coding Guidelines

### General

* Use modern C++ (C++20 features allowed)
* Prefer clear, readable code over clever code
* Avoid macros unless required for Win32

### Naming

* Types: `PascalCase`
* Functions: `camelCase`
* Member variables: `m_` prefix (e.g., `m_position`)
* Constants: `UPPER_CASE`

### Headers

* Use `#pragma once`
* Minimize includes in headers
* Prefer forward declarations where possible

### Error Handling

* Use HRESULT checks for Win32/DirectX calls
* Fail early with clear logging or assertions
* Do NOT silently ignore errors

### Memory Management

* Use RAII patterns
* Avoid manual memory management
* Release COM objects properly (e.g., `ComPtr` if used)

---

## Rendering Rules

* Target 60 FPS minimum
* Do not block the render loop
* Separate update and render logic
* Avoid allocations inside render loop
* Use double buffering where applicable

---

## Window Requirements (Phase 1)

* Borderless window
* Positioned to cover desktop area
* Initially NOT click-through
* Transparent background (where supported)

---

## Input Requirements (Phase 1)

* Support:

  * WASD
  * Arrow keys
* Simple polling is acceptable initially

---

## Character Requirements (Phase 1)

* Represent character as a simple shape (circle or rectangle)
* Position stored as float (x, y)
* Basic movement:

  * Constant speed
  * Frame-rate independent (delta time)

---

## Phase Rules

### IMPORTANT

Agents MUST ONLY implement the current phase.

### Phase 1 Scope

* Window creation
* Direct2D renderer setup
* Basic render loop
* Keyboard input (basic)
* Movable placeholder character

### Explicitly NOT allowed in Phase 1:

* Desktop icon detection
* Shell COM interfaces
* WorkerW manipulation
* Click-through windows
* Physics or collision with icons
* Animation systems

---

## Build Requirements

* Must compile with MSVC via CMake
* No external dependencies beyond Windows SDK
* Project must build cleanly without warnings where possible

---

## What Agents Should Do

When given a task:

1. Understand the current phase
2. Modify only relevant files
3. Keep code modular
4. Ensure project compiles
5. Avoid breaking existing functionality

---

## What Agents Should NOT Do

* Do NOT introduce large frameworks
* Do NOT rewrite entire architecture unless asked
* Do NOT implement future features early
* Do NOT add unnecessary complexity

---

## Future Phases (Reference Only)

* Desktop icon enumeration (Shell API)
* Collision with icons
* Click-through window behavior
* Animation system
* Multi-monitor support
* DirectComposition optimization

These are NOT to be implemented unless explicitly requested.

---

## Final Notes

* Prioritize correctness over feature count
* Keep iteration cycles small
* Every change should be testable

This project is meant to evolve incrementally into a polished, native Windows desktop experience.
