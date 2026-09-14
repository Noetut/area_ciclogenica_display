# Área Ciclogénica Display (Patrón Animation Engine)

A lightweight C++ Win32 application that projects calibrated pattern areas onto a secondary display (e.g. a 1080p projector or monitor), with an interactive calibration mode for aligning them to physical objects.

---

## Features

- **Multi-Monitor Support**: Automatically targets a secondary display (borderless window) with fallback to the primary monitor.
- **Double-Buffered Rendering**: Smooth, flicker-free 60 FPS rendering using Win32 GDI memory DCs.
- **Quadrilateral Projection Areas**: Each area has four independently movable corners, so a projector mounted off-axis can be compensated for. Axis-aligned areas still take a `FillRect` fast path for pixel-exact output.
- **Interactive Calibration Mode**: On-screen overlay for repositioning corners, moving whole areas, and creating, duplicating or deleting areas, with undo and persistent save.
- **Single Source of Truth**: `config/pattern_config.json` is read at start-up and rewritten by calibration. No coordinates are compiled into the binary.
- **Static Projection Mask**: Show mode projects the calibrated areas and changes only when a key toggles one. There is no timed animation; that is the job of the planned animation modules.

---

## Controls

### Show mode

| Key | Action |
|:---|:---|
| <kbd>Space</kbd> | Play / Pause animation sequence |
| <kbd>R</kbd> | Restart animation from beginning |
| <kbd>1</kbd> – <kbd>9</kbd> | Toggle the corresponding area ON / OFF |
| <kbd>A</kbd> | Turn **ALL** areas ON |
| <kbd>O</kbd> | Turn **ALL** areas OFF (clear to black) |
| <kbd>F1</kbd> | Enter calibration mode |
| <kbd>Esc</kbd> | Exit the application |

Which areas start lit comes from the `visible` flag of each area in the config, not from a hard-coded default.

### Calibration mode (<kbd>F1</kbd>)

| Key | Action |
|:---|:---|
| <kbd>Tab</kbd> / <kbd>Shift</kbd>+<kbd>Tab</kbd> | Next / previous area |
| <kbd>1</kbd> – <kbd>9</kbd> | Select area by index |
| Numpad <kbd>7</kbd> <kbd>9</kbd> <kbd>3</kbd> <kbd>1</kbd> | Select corner TL / TR / BR / BL (spatial layout) |
| Numpad <kbd>5</kbd> | Select the whole area (all four corners move together) |
| <kbd>Q</kbd> / <kbd>E</kbd> | Cycle corner backwards / forwards |
| Arrow keys | Move the target by 1 px |
| <kbd>Shift</kbd> + arrows | Move by 10 px |
| <kbd>Ctrl</kbd>+<kbd>Shift</kbd> + arrows | Move by 50 px |
| <kbd>N</kbd> | New area (200×150, centred on the canvas) |
| <kbd>D</kbd> | Duplicate the selected area, offset by 20 px |
| <kbd>Shift</kbd>+<kbd>Delete</kbd> | Delete the selected area |
| <kbd>Ctrl</kbd>+<kbd>Z</kbd> | Undo (50 levels) |
| <kbd>R</kbd> | Restore the selected area to its last saved state |
| <kbd>Ctrl</kbd>+<kbd>S</kbd> | Save to `config/pattern_config.json` |
| <kbd>L</kbd> | Reload from disk, discarding changes |
| <kbd>H</kbd> | Show / hide the information panel |
| <kbd>F1</kbd> | Back to show mode |
| <kbd>Esc</kbd> | Exit the application |

With NumLock off, Windows reports the navigation-cluster keys instead of the numpad ones, so <kbd>Home</kbd> / <kbd>PgUp</kbd> / <kbd>PgDn</kbd> / <kbd>End</kbd> / <kbd>Clear</kbd> also select corners.

Saving is done through a temporary file and keeps a `.bak` copy of the previous configuration, so an interrupted save cannot destroy a calibration.

---

## Configuration

`config/pattern_config.json` (schema 2.0) holds the canvas size and one entry per projection area. Only the corners are stored; width, height and centre are derived at load time, which is what keeps the geometry from drifting between two places.

```json
{
    "version": "2.0",
    "canvas": { "width": 1920, "height": 1080 },
    "areas": [
        {
            "id": 0,
            "name": "vinyl_turboviolencia",
            "description": "Left column, top square",
            "corners": [
                { "x": 85,  "y": 81  },
                { "x": 386, "y": 81  },
                { "x": 386, "y": 332 },
                { "x": 85,  "y": 332 }
            ],
            "color": [255, 255, 255],
            "visible": true
        }
    ]
}
```

Corner order is fixed and load-bearing: **top-left, top-right, bottom-right, bottom-left** (clockwise).

The legacy 1.x schema (`boxes` with `x` / `y` / `width` / `height`) is still accepted on load and converted to corners; the next save rewrites the file as 2.0.

`id` is a stable persisted identifier and is not the same thing as the position in the list: deleting an area shifts the indices but not the ids.

---

## Requirements & Build

### Requirements
- C++17 compatible compiler (MinGW-w64 GCC recommended on Windows)
- CMake 3.16+
- Ninja (optional, or standard MinGW Makefiles)

`nlohmann/json` 3.11.3 is vendored as a single header in `third_party/` (MIT licence); there is nothing to install.

### Build
```bash
cmake -B build -G Ninja
cmake --build build
```

### Run
```powershell
# Run on secondary monitor (default: monitor index 1)
.\build\PatronAnimation.exe 1

# Run on primary monitor
.\build\PatronAnimation.exe 0

# Start straight in calibration mode
.\build\PatronAnimation.exe 1 --calibrate

# Load a specific animation script
.\build\PatronAnimation.exe 0 --anim animations\sequential_wave.txt

# Use an explicit config file
.\build\PatronAnimation.exe 1 --config C:\path\to\pattern_config.json
```

The config file is resolved relative to the executable directory (`config/`, then `../config/`, then `../../config/`), so running from `build/` works without arguments. Pass `--help` for the full list.

Coordinates are absolute pixels for the calibration canvas. Running on a monitor whose resolution differs from `canvas` logs a warning and the areas will not line up; recalibrate for that display.

---

## Project Structure

```
area_ciclogenica_display/
├── animations/
│   └── sequential_wave.txt    # Text-based animation script
├── config/
│   └── pattern_config.json    # Single source of truth: canvas + area corners
├── images/
│   ├── Patrón.png             # Target reference pattern
│   └── pattern_detected_boxes.png # Visual overlay of detected zones
├── src/
│   ├── anim/                  # Text animation parser, sequencer & playback controller
│   ├── app/                   # Application lifecycle, mode dispatch & main loop
│   ├── calib/                 # Interactive calibration controller
│   ├── display/               # Monitor enumeration & borderless window creation
│   ├── model/                 # Geometry, projection areas, grid state, config I/O
│   ├── render/                # Double-buffered GDI rendering engine
│   ├── util/                  # Narrow/wide string helpers
│   └── main.cpp               # Entry point & argument parsing
├── third_party/
│   └── nlohmann/json.hpp      # Vendored JSON library (MIT)
├── CMakeLists.txt             # Build configuration
└── gemini.md                  # Detailed context and development tracker
```

### Where to change what

| To change… | Go to |
|:---|:---|
| Area coordinates | `config/pattern_config.json` — never in code |
| Nudge step sizes, undo depth, handle blink rate | `src/calib/CalibrationController.h`, `--- Tuning ---` block |
| Which key does what in calibration | `CalibrationController::HandleKey` |
| Show-mode keys, mode switching, quitting | `Application::HandleShowModeKey` / `HandleKeyDown` |
| Overlay colours, outline widths, HUD position | anonymous namespace at the top of `src/app/Application.cpp` |
| What the information panel shows | `CalibrationController::BuildHudText` |
| How the overlay is composed | `Application::RenderCalibrationOverlay` |
| GDI drawing primitives, HUD font | `src/render/RenderEngine.{h,cpp}` |
| JSON schema, migration, atomic save | `src/model/PatternConfig.cpp` |
| Quad maths (corners, bounding box, centre) | `src/model/Geometry.h` |
| Size of a new area, duplicate offset | anonymous namespace at the top of `src/model/PatternGrid.cpp` |
| Monitor selection, window creation | `src/display/DisplayManager.cpp` |
| Command-line arguments | `src/main.cpp` |
