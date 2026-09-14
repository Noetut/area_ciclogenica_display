#ifndef ANIMATION_TYPES_H
#define ANIMATION_TYPES_H

#include <string>
#include <vector>

enum class ActionType {
    AllOn,
    AllOff,
    TurnOn,
    TurnOff,
    Toggle,
    SetMask
};

struct AnimationAction {
    ActionType  type = ActionType::AllOn;
    int         targetIndex = -1;   // 0-based area index (-1 if unused)
    std::string targetName;         // Optional area name if referenced by name
    std::vector<bool> mask;         // Used for SetMask
};

struct AnimationFrame {
    double duration = 0.3;          // Duration in seconds to wait before next frame
    std::vector<AnimationAction> actions;
};

struct AnimationSequence {
    std::string name;
    bool        loop = true;
    double      defaultStep = 0.3;  // Default step duration in seconds
    std::vector<AnimationFrame> frames;

    bool Empty() const { return frames.empty(); }
    size_t FrameCount() const { return frames.size(); }
};

#endif // ANIMATION_TYPES_H
