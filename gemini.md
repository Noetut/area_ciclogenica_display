# Patrón Animation Engine - Context & Project Tracker

## Project Overview
This C++ application displays animated pattern graphics based on `Patrón.png` on a secondary monitor (1920x1080 resolution). The architecture is designed to grow modularly, allowing individual square control and custom blinking/sequencing animations.

---

## Current Status: Stage 1 Completed (Full Black Display)

### Completed Features:
- **Multi-Monitor Display Manager** (`src/display/DisplayManager.h/.cpp`):
  - Enumerates connected Windows displays via Win32 `EnumDisplayMonitors`.
  - Targets Monitor #2 (0-indexed `1`) with a 1920x1080 borderless frameless window (`WS_POPUP`).
  - Automatic fallback to Monitor #0 if only 1 screen is detected.
- **Double-Buffered Graphics Engine** (`src/render/RenderEngine.h/.cpp`):
  - Offscreen memory DC double-buffering using GDI (`BitBlt`) at 60 FPS without tearing or flicker.
  - Clears screen buffer to solid black (`RGB(0, 0, 0)`).
- **Engine Core & Main Loop** (`src/app/Application.h/.cpp` & `src/main.cpp`):
  - Delta-timed frame loop (~60 FPS cap).
  - Handles `ESC` key to gracefully terminate the application.
- **Build System** (`CMakeLists.txt`):
  - MinGW GCC + Ninja build configuration (`build/PatronAnimation.exe`).

---

## File & Directory Structure

```
d:\Proyectos\Área Ciclogénica Video\
├── Patrón.png               # Target pattern reference image containing white squares
├── CMakeLists.txt           # Build configuration targeting C++17, gdi32, user32
├── gemini.md                # Project tracker and context file for AI prompts
├── build/                   # Compiled binaries (build/PatronAnimation.exe)
└── src/
    ├── main.cpp              # Entry point
    ├── app/
    │   ├── Application.h    # Engine loop & timing
    │   └── Application.cpp  # Engine loop implementation
    ├── display/
    │   ├── DisplayManager.h # Win32 monitor enumeration & window creation
    │   └── DisplayManager.cpp
    └── render/
        ├── RenderEngine.h   # GDI double-buffer graphics renderer
        └── RenderEngine.cpp
```

---

## Build & Run Commands

### Environment / Compiler setup:
- Compiler: MinGW GCC 16.1.0 (`C:\Users\noeam\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin`)
- CMake: 4.4.3 (`C:\Program Files\CMake\bin`)

### Build Command:
```cmd
cmd /c "set PATH=C:\Program Files\CMake\bin;C:\Users\noeam\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH% && cmake -B build -G Ninja && cmake --build build"
```

### Execution:
```powershell
.\build\PatronAnimation.exe 1
```
*(Arg `1` = Monitor 2; Arg `0` = Monitor 1. Press `ESC` to exit).*

---

---

## Historical: Stage 2 & 2.1 (Pattern Grid & Full Blink Loop)

> The full blink loop described below was **removed in Stage 2.5**; show mode is
> now a static mask. This section is kept as a record of what was built then.

### Completed Features:
- **Image Analysis of `Patrón.png`**:
  - Image resolution: 1920x1080, solid black background with 9 white rectangles.
  - Subpixel edge detection & connected component analysis achieving 99.97% IoU.
- **Box Coordinates & Configuration**:
  - Configuration saved to `config/pattern_config.json`.
  - Annotated visual verification saved to `pattern_detected_boxes.png`.
  - C++ data model defined in `src/model/PatternBoxes.h` (`SquareData` struct, `BoxId` enum, and `DEFAULT_PATTERN_BOXES`).
- **Pattern Grid Manager (`src/model/PatternGrid.h/.cpp`)**:
  - Encapsulates square collection with methods to turn individual squares ON/OFF (by index or `BoxId`), query visibility, set custom colors, and toggle all squares.
- **Double-Buffered GDI Pattern Rendering (`src/render/RenderEngine.h/.cpp`)**:
  - `RenderSquares(squares)` fills active squares onto memory DC with cached GDI brushes at 60 FPS without memory leaks.
- **1-Second Full Blink Animation Loop (`src/app/Application.h/.cpp`)**:
  - Automatically alternates all squares between ON (white) and OFF (black) every 1.0 second.
  - Interactive keyboard controls: <kbd>Space</kbd> to pause/resume blink loop, <kbd>1</kbd>–<kbd>9</kbd> to toggle individual squares, <kbd>A</kbd> for all ON, <kbd>C</kbd> for all OFF, <kbd>ESC</kbd> to exit.

### Calibrated Area Mappings:
The coordinates deliberately live in **one place only**: `config/pattern_config.json`.
They used to be duplicated in a `DEFAULT_PATTERN_BOXES` table in `PatternBoxes.h`
and in this document; that duplication was removed in Stage 2.5. The list below is
an orientation aid, not a specification — read the config for the actual geometry.

| Index | Name | Position |
|:---|:---|:---|
| 0 | `vinyl_turboviolencia` | left column, top |
| 1 | `vinyl_cancionero` | left column, middle |
| 2 | `vinyl_mecharadio` | left column, bottom |
| 3 | `frame_aneto` | upper group, left |
| 4 | `frame_brais_clouds` | upper group, middle |
| 5 | `frame_brais_eyes` | upper group, right |
| 6 | `frame_green_screen` | lower group, left |
| 7 | `frame_pepus` | lower group, middle |
| 8 | `frame_margarita` | lower group, right |

The `BoxId` enum was removed: with areas created and deleted at runtime, a
compile-time enum of nine fixed identifiers no longer holds. Areas are addressed
by list index (transient) or by the persisted `id` / `name` (stable).

---

## Current Status: Stage 2.5 Completed (Calibration Mode & Single Source of Truth)

### Completed Features:
- **Quadrilateral geometry model** (`src/model/Geometry.h`):
  - `Quad` with four independently movable corners in fixed clockwise order
    (TL, TR, BR, BL), so an off-axis projector can be compensated for.
  - Width, height and centre are **derived**, never stored.
  - `IsAxisAlignedRect()` gates a `FillRect` fast path, keeping uncalibrated
    areas pixel-identical to the Stage 2 output (GDI's polygon fill convention
    differs from `FillRect` at the borders).
- **Single source of truth** (`src/model/PatternConfig.{h,cpp}`):
  - `config/pattern_config.json` (schema 2.0) is loaded at start-up and
    rewritten by calibration. `DEFAULT_PATTERN_BOXES` was removed from the
    headers; no coordinates are compiled in.
  - Legacy schema 1.x (`boxes` with x/y/width/height) is still read and
    converted to corners; the next save rewrites the file as 2.0.
  - Derived fields (`width`, `height`, `center_x`, `center_y`, `total_boxes`)
    are no longer persisted, since they were internal duplication.
  - Saves are atomic (temp file plus `MoveFileExW`) and keep a `.bak` copy.
  - File I/O goes through `CreateFileW` rather than `std::ifstream`, because
    wide-path stream constructors are an MSVC extension and this builds on MinGW.
  - Config path is resolved relative to the executable directory, with a
    `--config` override.
- **Calibration mode** (`src/calib/CalibrationController.{h,cpp}`):
  - Per-corner and whole-area nudging at 1 / 10 / 50 px.
  - Create, duplicate and delete areas; delete requires `Shift` as confirmation.
  - 50-level undo; dirty state is computed by comparing against the on-disk
    snapshot, so undoing back to the saved state clears the warning.
  - Save, reload-from-disk and per-area reset.
- **Calibration overlay** (`RenderEngine` additions):
  - `FillQuad`, `FillQuadHalftone` (an 8x8 monochrome pattern brush stands in
    for the alpha channel GDI does not have), `DrawQuadOutline`, `DrawHandle`,
    `DrawCross`, `DrawHudText`.
  - Cached pens, brushes and font; the overlay redraws every outline each frame.
  - Corner handles, blinking active handle, centre cross, per-area index labels
    and an on-screen information panel. None of it renders in show mode.
- **Full blink loop removed** (was Stage 2.1):
  - `m_isFullBlinkActive`, `m_blinkTimer`, `m_blinkInterval`, `m_allAreasOn`,
    `SetFullBlinkActive`, `IsFullBlinkActive`, `SetBlinkInterval` and the
    <kbd>Space</kbd> binding are all gone. `Application::Update` has no
    time-driven behaviour in show mode; the projection is a static mask.
  - Initial visibility now comes from each area's persisted `visible` flag
    instead of being forced on at start-up.
  - Switching modes no longer touches visibility in either direction. The
    overlay already draws every area regardless of its flag, so lighting them
    up for editing was unnecessary — and it meant a save from calibration
    could silently turn hidden areas back on.
  - The blinking corner handle in calibration is a separate thing (a UI
    indicator, `m_handleBlinkOn`) and was kept.
- **Dependency**: `nlohmann/json` 3.11.3 vendored at `third_party/nlohmann/json.hpp` (MIT).
- **Two pre-existing defects fixed along the way**:
  - The projection window now claims keyboard focus (`SetForegroundWindow` /
    `SetFocus`), which calibration depends on.
  - `PeekMessageW` results are filtered by window handle before key handling.
  - Duplicate `ESC` handling in `DisplayManager::WindowProc` was removed;
    `Application` owns key handling.

### Verification status:
**Builds clean** with MinGW GCC 16.1.0 (WinLibs UCRT) + CMake + Ninja: all
8 objects compile and link to `build/PatronAnimation.exe` with no errors and no
warnings. Argument parsing and config path resolution confirmed at runtime via
`--help`.

Behaviour was additionally verified by compiling the real sources against a
`windows.h` test double under Linux g++ (the double stubs GDI, so it exercises
logic only):
- 275 assertions over config load/save, v1 migration, backup and atomic replace,
  defensive parsing, grid CRUD and geometry edge cases.
- 118 assertions driving `CalibrationController` through scripted key sequences,
  including undo coalescing under simulated key auto-repeat.
- All nine calibrated areas round-trip exactly, their derived centres match the
  Stage 2 values to the decimal, and all remain axis-aligned.
- `-fsyntax-only -Wall -Wextra` is clean on every source file.

**Still unverified: everything visual.** The overlay appearance, the keystoned
quad fill, window placement on the projector and frame pacing have not been seen
on screen. That needs a run against the real display.

### Build environment note:
The project must live on a native Windows path. Building from the WSL filesystem
over `\\wsl.localhost\...` fails: CMake runs its link step through
`cmd.exe /C "cd . && ..."`, `cmd.exe` refuses a UNC working directory and falls
back to `C:\Windows`, and `ld` then cannot write the output. `net use` cannot map
the WSL 9P share (error 67) and `subst` does not accept UNC paths either, so
there is no workaround short of a local path.

---

## File & Directory Structure

```
area_ciclogenica_display/
├── images/
│   ├── Patrón.png               # Target pattern reference image (1920x1080)
│   └── pattern_detected_boxes.png # Visual overlay of detected boxes with IDs & labels
├── CMakeLists.txt           # Build configuration targeting C++17, gdi32, user32
├── gemini.md                # Project tracker and context file
├── config/
│   └── pattern_config.json  # SINGLE SOURCE OF TRUTH: canvas + area corners
├── third_party/
│   └── nlohmann/json.hpp    # Vendored JSON library (MIT)
├── build/                   # Compiled binaries (build/PatronAnimation.exe)
└── src/
    ├── main.cpp             # Entry point & argument parsing
    ├── app/
    │   ├── Application.h    # Engine loop, mode dispatch, key handling
    │   └── Application.cpp  # Engine loop and calibration overlay composition
    ├── calib/
    │   ├── CalibrationController.h   # Selection, nudging, CRUD, undo, persistence
    │   └── CalibrationController.cpp
    ├── display/
    │   ├── DisplayManager.h # Win32 monitor enumeration & window creation
    │   └── DisplayManager.cpp
    ├── model/
    │   ├── Geometry.h       # Point2i, Corner, Quad (4 independent corners)
    │   ├── ProjectionArea.h # ProjectionArea struct (replaces SquareData)
    │   ├── PatternConfig.h  # JSON load/save, v1 migration, path resolution
    │   ├── PatternConfig.cpp
    │   ├── PatternGrid.h    # Area collection: visibility, colour, CRUD, lookup
    │   └── PatternGrid.cpp
    ├── render/
    │   ├── RenderEngine.h   # GDI double buffer + quad and overlay primitives
    │   └── RenderEngine.cpp
    └── util/
        └── StringUtil.h     # UTF-8 / UTF-16 / ANSI conversions
```

---

## Future Roadmap / Next Steps

1. **Build and validate on Windows**: compile with MinGW and check the overlay,
   the quad rendering path and the save cycle on the real projector.
2. **Mouse-driven calibration** (deferred on purpose): drag corners directly with
   hit-testing on the handles. Needs `WM_MOUSEMOVE` / `WM_LBUTTONDOWN` routing,
   which in turn means restructuring how `Application` sniffs messages.
3. **Stage 3: Animation Modules (`IAnimationModule`)**:
   - Abstract animation interface `IAnimationModule` (`Initialize`, `Update`, `Render`).
   - Implement animation drivers (e.g. `AreaBlinkModule`, `PulseModule`, `SequenceModule`, `ColorFadeModule`).
   - Sequencer / Timeline controller to chain animations across areas.
   - The reserved `opacity`, `isBlinking`, `blinkRate` and `phase` fields on
     `ProjectionArea` are the intended hooks.

### Known gaps / deliberate omissions
- **No resolution scaling.** Coordinates are absolute pixels for the canvas in
  the config; a different monitor logs a warning and needs recalibrating.
- **No reference image underlay** during calibration: GDI alone cannot decode
  PNG, that would need WIC or GDI+.
- **No vsync.** `BitBlt` onto a persistent `GetDC` outside `WM_PAINT` can tear
  despite the double buffer.
- **Show mode still redraws at 60 FPS** even though the mask is now static.
  Harmless but wasteful; a dirty-flag render would need care to avoid stale
  frames when the window is damaged.

### Where the tunable values live
Grouped on purpose, so behaviour can be changed without hunting through code:
- `CalibrationController.h`, `--- Tuning ---` block: nudge step sizes, undo
  depth, nudge-coalescing window, corner-handle blink period.
- `Application.cpp`, anonymous namespace: target frame rate, overlay palette
  and overlay layout in pixels.
- `PatternGrid.cpp`, anonymous namespace: size of an area created with `N`,
  offset of a copy made with `D`.
- `RenderEngine.cpp`, anonymous namespace: HUD font and padding, halftone
  pattern bits.
- `config/pattern_config.json`: all geometry. Never in code.

