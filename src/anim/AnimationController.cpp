#include "AnimationController.h"

#include <iostream>
#include "AnimationParser.h"
#include "model/PatternGrid.h"

AnimationController::AnimationController()
    : m_currentFrameIndex(0)
    , m_frameTimer(0.0)
    , m_isPlaying(false)
{
}

AnimationController::~AnimationController() {
}

bool AnimationController::LoadFromFile(const std::string& filePath, std::string& outError) {
    AnimationSequence seq;
    if (!AnimationParser::LoadFromFile(filePath, seq, outError)) {
        return false;
    }

    m_filePath = filePath;
    return LoadFromSequence(seq);
}

bool AnimationController::LoadFromSequence(const AnimationSequence& sequence) {
    m_sequence = sequence;
    m_currentFrameIndex = 0;
    m_frameTimer = 0.0;
    m_isPlaying = !m_sequence.Empty();

    std::cout << "[Animation] Loaded sequence '" << m_sequence.name
              << "' (" << m_sequence.frames.size() << " frames, loop="
              << (m_sequence.loop ? "true" : "false") << ")" << std::endl;
    return true;
}

void AnimationController::Play() {
    if (!m_sequence.Empty()) {
        m_isPlaying = true;
    }
}

void AnimationController::Pause() {
    m_isPlaying = false;
}

void AnimationController::TogglePlayPause() {
    if (m_sequence.Empty()) return;
    m_isPlaying = !m_isPlaying;
    std::cout << "[Animation] " << (m_isPlaying ? "PLAYING" : "PAUSED")
              << " [Frame " << (m_currentFrameIndex + 1) << "/"
              << m_sequence.frames.size() << "]" << std::endl;
}

void AnimationController::Stop() {
    m_isPlaying = false;
    m_currentFrameIndex = 0;
    m_frameTimer = 0.0;
}

void AnimationController::Restart(PatternGrid& grid) {
    if (m_sequence.Empty()) return;
    m_currentFrameIndex = 0;
    m_frameTimer = 0.0;
    m_isPlaying = true;
    ApplyFrame(m_sequence.frames[0], grid);
    std::cout << "[Animation] Restarted sequence from frame 1." << std::endl;
}

void AnimationController::Update(double deltaTime, PatternGrid& grid) {
    if (!m_isPlaying || m_sequence.Empty()) return;

    m_frameTimer += deltaTime;

    while (m_isPlaying && !m_sequence.Empty() &&
           m_frameTimer >= m_sequence.frames[m_currentFrameIndex].duration) {

        m_frameTimer -= m_sequence.frames[m_currentFrameIndex].duration;
        ++m_currentFrameIndex;

        if (m_currentFrameIndex >= m_sequence.frames.size()) {
            if (m_sequence.loop) {
                m_currentFrameIndex = 0;
            } else {
                m_currentFrameIndex = m_sequence.frames.size() - 1;
                m_isPlaying = false;
                std::cout << "[Animation] Sequence finished (non-looping)." << std::endl;
                break;
            }
        }

        ApplyFrame(m_sequence.frames[m_currentFrameIndex], grid);
    }
}

void AnimationController::ApplyFrame(const AnimationFrame& frame, PatternGrid& grid) {
    for (const auto& action : frame.actions) {
        switch (action.type) {
        case ActionType::AllOn:
            grid.SetAllVisible(true);
            break;

        case ActionType::AllOff:
            grid.SetAllVisible(false);
            break;

        case ActionType::TurnOn: {
            int idx = action.targetIndex;
            if (idx < 0 && !action.targetName.empty()) {
                idx = grid.FindIndexByName(action.targetName);
            }
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.SetAreaVisible(idx, true);
            }
            break;
        }

        case ActionType::TurnOff: {
            int idx = action.targetIndex;
            if (idx < 0 && !action.targetName.empty()) {
                idx = grid.FindIndexByName(action.targetName);
            }
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.SetAreaVisible(idx, false);
            }
            break;
        }

        case ActionType::Toggle: {
            int idx = action.targetIndex;
            if (idx < 0 && !action.targetName.empty()) {
                idx = grid.FindIndexByName(action.targetName);
            }
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.ToggleArea(idx);
            }
            break;
        }

        case ActionType::SetMask: {
            for (size_t i = 0; i < action.mask.size() && i < grid.GetCount(); ++i) {
                grid.SetAreaVisible(static_cast<int>(i), action.mask[i]);
            }
            break;
        }
        }
    }
}
