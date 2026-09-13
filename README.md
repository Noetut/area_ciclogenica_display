# Área Ciclogénica Display (Patrón Animation Engine)

A lightweight C++ Win32 application designed to project and display calibrated animated patterns onto a secondary display (e.g., a 1080p projector or monitor).

---

## Features

- **Multi-Monitor Support**: Automatically targets a secondary display (1920x1080 borderless window) with fallback to the primary monitor.
- **Double-Buffered Rendering**: Smooth, flicker-free 60 FPS rendering using Win32 GDI memory DCs.
- **Calibrated Pattern Grid**: Maps 9 precise rectangular display zones (vinyl slots and frames) extracted from `Patrón.png`.
- **Interactive Controls & Animation**: Built-in 1-second blink loop with manual keyboard controls to toggle individual zones or all zones simultaneously.

---

## Controls

| Key | Action |
|:---|:---|
| <kbd>Space</kbd> | Pause / Resume the 1-second full blink loop |
| <kbd>1</kbd> - <kbd>9</kbd> | Toggle corresponding pattern zone ON / OFF |
| <kbd>A</kbd> | Turn **ALL** zones ON |
| <kbd>C</kbd> | Turn **ALL** zones OFF (clear to black) |
| <kbd>ESC</kbd> | Exit the application |

---

## Requirements & Build

### Requirements
- C++17 compatible compiler (MinGW-w64 GCC recommended on Windows)
- CMake 3.16+
- Ninja (optional, or standard MinGW Makefiles)

### Build
```bash
cmake -B build -G Ninja
cmake --build build
```

### Run
```powershell
# Run on secondary monitor (default: monitor index 1)
.\build\PatronAnimation.exe 1

# Run on primary monitor (monitor index 0)
.\build\PatronAnimation.exe 0
```

---

## Project Structure

```
area_ciclogenica_display/
├── config/
│   └── pattern_config.json    # Calibrated pixel coordinates and dimensions
├── images/
│   ├── Patrón.png             # Target reference pattern
│   └── pattern_detected_boxes.png # Visual overlay of detected zones
├── src/
│   ├── app/                   # Application lifecycle & main loop
│   ├── display/               # Monitor enumeration & borderless window creation
│   ├── model/                 # Pattern box definitions & grid state management
│   ├── render/                # Double-buffered GDI rendering engine
│   └── main.cpp               # Entry point
├── CMakeLists.txt             # Build configuration
└── gemini.md                  # Detailed context and development tracker
```
