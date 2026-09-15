#ifndef ANIMATION_TYPES_H
#define ANIMATION_TYPES_H

#include <windows.h>
#include <string>
#include <vector>

enum class ActionType {
    AllOn,
    AllOff,
    TurnOn,
    TurnOff,
    Toggle,
    SetMask,
    SetImage,
    ClearImage,
    ClearAllImages,
    PreloadImage,
    SetText,
    ClearText,
    ClearAllTexts
};

struct AnimationAction {
    ActionType  type = ActionType::AllOn;
    int         targetIndex = -1;   // 0-based area index (-1 if unused)
    int         targetId = -1;      // Area id from config (-1 if unused)
    std::string targetName;         // Optional area name if referenced by name
    std::string imagePath;          // Image path / name for SetImage / Preload
    std::string text;               // Text content for SetText
    std::string fontFace = "Arial"; // Font family name
    int         fontSize = 32;      // Font size in points
    COLORREF    textColor = RGB(255, 255, 255); // Color of text glyphs
    std::vector<bool> mask;         // Used for SetMask
};

struct AnimationFrame {
    double duration = 0.3;          // Duration in seconds to wait before next frame
    bool   waitForClick = false;    // If true, pauses at this frame waiting for mouse click / cue
    std::string segmentName;        // Optional cue/segment name
    std::vector<AnimationAction> actions;
};

struct AnimationSequence {
    std::string name;
    bool        loop = true;
    bool        resetImages = true; // Clear image overlays to default white on startup
    double      defaultStep = 0.3;  // Default step duration in seconds
    std::vector<std::string>    preloadImages; // Explicitly preloaded image list
    std::vector<AnimationFrame> frames;

    bool Empty() const { return frames.empty(); }
    size_t FrameCount() const { return frames.size(); }
};

#endif // ANIMATION_TYPES_H
