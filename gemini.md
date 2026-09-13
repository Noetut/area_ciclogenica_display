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

## Current Status: Stage 2 & 2.1 Completed (Pattern Grid & Full Blink Loop)

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

### Calibrated Box Mappings:
| ID | Enum (`BoxId`) | Name | Position | X, Y, W, H |
|:---|:---|:---|:---|:---|
| 0 | `VinylTurboviolencia` | `vinyl_turboviolencia` | `left_top` | 85, 81, 301, 251 |
| 1 | `VinylCancionero` | `vinyl_cancionero` | `left_middle` | 90, 390, 305, 248 |
| 2 | `VinylMecharadio` | `vinyl_mecharadio` | `left_bottom` | 90, 691, 301, 256 |
| 3 | `FrameAneto` | `frame_aneto` | `center_upper_left` | 561, 278, 260, 163 |
| 4 | `FrameBraisClouds` | `frame_brais_clouds` | `center_upper_middle` | 893, 304, 187, 114 |
| 5 | `FrameBraisEyes` | `frame_brais_eyes` | `center_upper_right` | 1181, 309, 263, 166 |
| 6 | `FrameGreenScreen` | `frame_green_screen` | `center_lower_left` | 644, 532, 215, 233 |
| 7 | `FramePepus` | `frame_pepus` | `center_lower_middle` | 985, 475, 110, 134 |
| 8 | `FrameMargarita` | `frame_margarita` | `center_lower_right` | 1170, 535, 135, 156 |

---

## File & Directory Structure

```
d:\Proyectos\Área Ciclogénica Video\
├── Patrón.png               # Target pattern reference image (1920x1080)
├── pattern_detected_boxes.png # Visual overlay of detected boxes with IDs & labels
├── CMakeLists.txt           # Build configuration targeting C++17, gdi32, user32
├── gemini.md                # Project tracker and context file
├── config/
│   └── pattern_config.json  # Box pixel coordinates, dimensions, and centers
├── build/                   # Compiled binaries (build/PatronAnimation.exe)
└── src/
    ├── main.cpp             # Entry point
    ├── app/
    │   ├── Application.h    # Engine loop & timing, full blink timer, key handling
    │   └── Application.cpp  # Engine loop implementation
    ├── display/
    │   ├── DisplayManager.h # Win32 monitor enumeration & window creation
    │   └── DisplayManager.cpp
    ├── model/
    │   ├── PatternBoxes.h   # SquareData struct, BoxId enum, default box array
    │   ├── PatternGrid.h    # Square state manager, visibility & color methods
    │   └── PatternGrid.cpp
    └── render/
        ├── RenderEngine.h   # GDI double-buffer graphics renderer (RenderSquares)
        └── RenderEngine.cpp
```

---

## Future Roadmap / Next Steps

1. **Stage 3: Animation Modules (`IAnimationModule`)**:
   - Abstract animation interface `IAnimationModule` (`Initialize`, `Update`, `Render`).
   - Implement animation drivers (e.g. `SquareBlinkModule`, `PulseModule`, `SequenceModule`, `ColorFadeModule`).
   - Sequencer / Timeline controller to chain animations across squares.

