#ifndef PATTERN_BOXES_H
#define PATTERN_BOXES_H

#include <windows.h>
#include <string>
#include <vector>

// Named identifiers for each calibrated box/frame in the pattern
enum class BoxId {
    VinylTurboviolencia = 0,
    VinylCancionero     = 1,
    VinylMecharadio     = 2,
    FrameAneto          = 3,
    FrameBraisClouds    = 4,
    FrameBraisEyes      = 5,
    FrameGreenScreen    = 6,
    FramePepus          = 7,
    FrameMargarita      = 8,
    Count               = 9
};

struct SquareData {
    int id;
    const char* name;
    RECT rect;          // left, top, right, bottom
    int width;
    int height;
    float centerX;
    float centerY;
    COLORREF color;     // Default RGB(255, 255, 255)
    float opacity;      // 0.0f to 1.0f
    bool isVisible;     // Visibility toggle
    bool isBlinking;    // Blink state
    float blinkRate;    // Frequency in Hz
    float phase;        // Animation phase
};

// Default pre-calibrated boxes extracted from Patrón.png (1920x1080)
inline const std::vector<SquareData> DEFAULT_PATTERN_BOXES = {
    // ID 0: left_top
    { 0, "vinyl_turboviolencia", { 85,   81,  85 + 301,  81 + 251  }, 301, 251, 235.5f, 206.5f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 1: left_middle
    { 1, "vinyl_cancionero",     { 90,   390, 90 + 305,  390 + 248 }, 305, 248, 242.5f, 514.0f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 2: left_bottom
    { 2, "vinyl_mecharadio",     { 90,   691, 90 + 301,  691 + 256 }, 301, 256, 240.5f, 819.0f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 3: center_upper_left
    { 3, "frame_aneto",          { 561,  278, 561 + 260, 278 + 163 }, 260, 163, 691.0f, 359.5f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 4: center_upper_middle
    { 4, "frame_brais_clouds",   { 893,  304, 893 + 187, 304 + 114 }, 187, 114, 986.5f, 361.0f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 5: center_upper_right
    { 5, "frame_brais_eyes",     { 1181, 309, 1181 + 263, 309 + 166}, 263, 166, 1312.5f, 392.0f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 6: center_lower_left
    { 6, "frame_green_screen",   { 644,  532, 644 + 215, 532 + 233 }, 215, 233, 751.5f, 648.5f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 7: center_lower_middle
    { 7, "frame_pepus",          { 985,  475, 985 + 110, 475 + 134 }, 110, 134, 1040.0f, 542.0f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f },
    // ID 8: center_lower_right
    { 8, "frame_margarita",      { 1170, 535, 1170 + 135, 535 + 156}, 135, 156, 1237.5f, 613.0f, RGB(255, 255, 255), 1.0f, true, false, 1.0f, 0.0f }
};

#endif // PATTERN_BOXES_H
