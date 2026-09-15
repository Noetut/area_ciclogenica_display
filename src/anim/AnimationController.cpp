#include "AnimationController.h"

#include <iostream>
#include <vector>
#include <windows.h>
#include "AnimationParser.h"
#include "model/PatternGrid.h"
#include "render/RenderEngine.h"
#include "util/StringUtil.h"

AnimationController::AnimationController()
    : m_currentFrameIndex(0)
    , m_frameTimer(0.0)
    , m_isPlaying(false)
    , m_isWaitingForClick(false)
    , m_frameApplied(false)
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
    m_isWaitingForClick = false;
    m_frameApplied = false;

    std::cout << "[Animation] Loaded sequence '" << m_sequence.name
              << "' (" << m_sequence.frames.size() << " frames, loop="
              << (m_sequence.loop ? "true" : "false") << ")" << std::endl;
    return true;
}

void AnimationController::PreloadImages(RenderEngine& renderEngine) {
    for (const auto& img : m_sequence.preloadImages) {
        if (!img.empty()) {
            renderEngine.GetOrLoadImage(img);
        }
    }

    for (const auto& frame : m_sequence.frames) {
        for (const auto& action : frame.actions) {
            if ((action.type == ActionType::SetImage || action.type == ActionType::PreloadImage) &&
                !action.imagePath.empty()) {
                renderEngine.GetOrLoadImage(action.imagePath);
            }
        }
    }
}

void AnimationController::Play() {
    if (!m_sequence.Empty()) {
        m_isPlaying = true;
        m_videoPlayer.Play();
    }
}

void AnimationController::Pause() {
    m_isPlaying = false;
    m_videoPlayer.Pause();
}

void AnimationController::TogglePlayPause() {
    if (m_sequence.Empty()) return;
    m_isPlaying = !m_isPlaying;
    if (m_isPlaying) {
        m_videoPlayer.Play();
    } else {
        m_videoPlayer.Pause();
    }
    std::cout << "[Animation] " << (m_isPlaying ? "PLAYING" : "PAUSED")
              << " [Frame " << (m_currentFrameIndex + 1) << "/"
              << m_sequence.frames.size() << "]" << std::endl;
}

void AnimationController::Stop() {
    m_isPlaying = false;
    m_isWaitingForClick = false;
    m_frameApplied = false;
    m_currentFrameIndex = 0;
    m_frameTimer = 0.0;
    m_videoPlayer.Stop();
}

void AnimationController::Restart(PatternGrid& grid) {
    if (m_sequence.Empty()) return;
    m_videoPlayer.Close();
    if (m_sequence.resetImages) {
        grid.ClearAllImages();
        grid.ClearAllTexts();
    }
    m_currentFrameIndex = 0;
    m_frameTimer = 0.0;
    m_isPlaying = true;
    m_isWaitingForClick = false;
    m_frameApplied = true;
    ApplyFrame(m_sequence.frames[0], grid);
    if (m_sequence.frames[0].waitForClick) {
        m_isWaitingForClick = true;
        std::cout << "[Animation] Paused waiting for click [Frame 1/"
                  << m_sequence.frames.size() << "]" << std::endl;
    }
    std::cout << "[Animation] Restarted sequence from frame 1." << std::endl;
}

void AnimationController::TriggerClick(PatternGrid& grid) {
    if (m_sequence.Empty()) return;

    if (m_isWaitingForClick) {
        m_isWaitingForClick = false;
        m_frameTimer = 0.0;
        ++m_currentFrameIndex;

        if (m_currentFrameIndex >= m_sequence.frames.size()) {
            if (m_sequence.loop) {
                m_currentFrameIndex = 0;
            } else {
                m_currentFrameIndex = m_sequence.frames.size() - 1;
                m_isPlaying = false;
                std::cout << "[Animation] Sequence finished (non-looping)." << std::endl;
                return;
            }
        }

        ApplyFrame(m_sequence.frames[m_currentFrameIndex], grid);
        std::cout << "[Animation] Mouse click trigger! Advanced to frame "
                  << (m_currentFrameIndex + 1) << "/" << m_sequence.frames.size() << std::endl;

        if (m_sequence.frames[m_currentFrameIndex].waitForClick) {
            m_isWaitingForClick = true;
            std::cout << "[Animation] Paused waiting for click [Frame "
                      << (m_currentFrameIndex + 1) << "/" << m_sequence.frames.size() << "]" << std::endl;
        }
    } else {
        std::cout << "[Animation] Mouse click received while playing (Frame "
                  << (m_currentFrameIndex + 1) << "/" << m_sequence.frames.size() << ")" << std::endl;
    }
}

void AnimationController::Update(double deltaTime, PatternGrid& grid) {
    if (m_videoPlayer.IsPlaying()) {
        m_videoPlayer.Update(deltaTime);
    }

    if (!m_isPlaying || m_sequence.Empty()) return;

    // Apply first frame on initial update if not applied yet
    if (!m_frameApplied) {
        m_frameApplied = true;
        if (m_sequence.resetImages) {
            grid.ClearAllImages();
            grid.ClearAllTexts();
        }
        ApplyFrame(m_sequence.frames[0], grid);
        if (m_sequence.frames[0].waitForClick) {
            m_isWaitingForClick = true;
            std::cout << "[Animation] Paused waiting for click [Frame 1/"
                      << m_sequence.frames.size() << "]" << std::endl;
            return;
        }
    }

    if (m_isWaitingForClick) return;

    m_frameTimer += deltaTime;

    while (m_isPlaying && !m_sequence.Empty() && !m_isWaitingForClick &&
           m_frameTimer >= m_sequence.frames[m_currentFrameIndex].duration) {

        m_frameTimer -= m_sequence.frames[m_currentFrameIndex].duration;
        ++m_currentFrameIndex;

        if (m_currentFrameIndex >= m_sequence.frames.size()) {
            if (m_sequence.loop) {
                m_currentFrameIndex = 0;
                if (m_sequence.resetImages) {
                    grid.ClearAllImages();
                    grid.ClearAllTexts();
                }
            } else {
                m_currentFrameIndex = m_sequence.frames.size() - 1;
                m_isPlaying = false;
                std::cout << "[Animation] Sequence finished (non-looping)." << std::endl;
                break;
            }
        }

        ApplyFrame(m_sequence.frames[m_currentFrameIndex], grid);

        if (m_sequence.frames[m_currentFrameIndex].waitForClick) {
            m_isWaitingForClick = true;
            std::cout << "[Animation] Paused waiting for click [Frame "
                      << (m_currentFrameIndex + 1) << "/" << m_sequence.frames.size() << "]" << std::endl;
            break;
        }
    }
}

namespace {

std::wstring ResolveVideoPath(const std::string& path) {
    if (path.empty()) return std::wstring();

    std::vector<std::string> candidates;
    candidates.push_back(path);
    candidates.push_back(path + ".mp4");
    candidates.push_back("images/" + path);
    candidates.push_back("images/" + path + ".mp4");
    candidates.push_back("../images/" + path);
    candidates.push_back("../images/" + path + ".mp4");
    candidates.push_back("../../images/" + path);
    candidates.push_back("../../images/" + path + ".mp4");

    wchar_t exeBuf[MAX_PATH] = { 0 };
    DWORD written = GetModuleFileNameW(NULL, exeBuf, MAX_PATH);
    if (written > 0 && written < MAX_PATH) {
        std::wstring exePath(exeBuf);
        size_t slash = exePath.find_last_of(L"\\/");
        if (slash != std::wstring::npos) {
            std::string exeDir = WideToUtf8(exePath.substr(0, slash));
            candidates.push_back(exeDir + "/" + path);
            candidates.push_back(exeDir + "/" + path + ".mp4");
            candidates.push_back(exeDir + "/images/" + path);
            candidates.push_back(exeDir + "/images/" + path + ".mp4");
            candidates.push_back(exeDir + "/../images/" + path);
            candidates.push_back(exeDir + "/../images/" + path + ".mp4");
        }
    }

    for (const auto& cand : candidates) {
        std::wstring wide = Utf8ToWide(cand);
        DWORD attr = GetFileAttributesW(wide.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            return wide;
        }
    }
    return std::wstring();
}

int ResolveTargetIndex(const AnimationAction& action, const PatternGrid& grid) {
    int idx = -1;
    if (action.targetId >= 0) {
        idx = grid.FindIndexById(action.targetId);
    }
    if (idx < 0 && !action.targetName.empty()) {
        idx = grid.FindIndexByName(action.targetName);
    }
    if (idx < 0 && action.targetIndex >= 0) {
        idx = action.targetIndex;
    }
    return idx;
}
} // namespace

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
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.SetAreaVisible(idx, true);
            }
            break;
        }

        case ActionType::TurnOff: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.SetAreaVisible(idx, false);
            }
            break;
        }

        case ActionType::Toggle: {
            int idx = ResolveTargetIndex(action, grid);
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

        case ActionType::SetImage: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.SetAreaImage(idx, action.imagePath);
                grid.SetAreaVisible(idx, true);
            }
            break;
        }

        case ActionType::ClearImage: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.ClearAreaImage(idx);
            }
            break;
        }

        case ActionType::ClearAllImages: {
            grid.ClearAllImages();
            break;
        }

        case ActionType::SetText: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.SetAreaText(idx, action.text, action.fontFace, action.fontSize, action.textColor);
                grid.SetAreaVisible(idx, true);
            }
            break;
        }

        case ActionType::ClearText: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                grid.ClearAreaText(idx);
            }
            break;
        }

        case ActionType::ClearAllTexts: {
            grid.ClearAllTexts();
            break;
        }

        case ActionType::PreloadImage: {
            // Already preloaded at startup, no runtime operation needed
            break;
        }

        case ActionType::SetBackgroundVideo: {
            std::wstring vpath = ResolveVideoPath(action.videoPath);
            if (!vpath.empty()) {
                m_videoPlayer.Open(vpath, true);
            } else {
                std::cerr << "[AnimationController] Could not resolve background video: " << action.videoPath << std::endl;
            }
            break;
        }

        case ActionType::StopBackgroundVideo: {
            m_videoPlayer.Close();
            break;
        }
        }
    }
}
