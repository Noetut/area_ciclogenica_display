#include "AnimationController.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <windows.h>
#include "AnimationParser.h"
#include "model/PatternGrid.h"
#include "render/RenderEngine.h"
#include "util/StringUtil.h"

AnimationController::AnimationController()
    : m_activeVideoPlayer(nullptr)
    , m_currentFrameIndex(0)
    , m_frameTimer(0.0)
    , m_isPlaying(false)
    , m_isWaitingForClick(false)
    , m_frameApplied(false)
{
}

AnimationController::~AnimationController() {
    m_activeVideoPlayer = nullptr;
    m_videoPlayers.clear();
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

    m_activePalpitations.clear();
    if (m_activeVideoPlayer) {
        m_activeVideoPlayer->Stop();
        m_activeVideoPlayer = nullptr;
    }

    std::cout << "[Animation] Loaded sequence '" << m_sequence.name
              << "' (" << m_sequence.frames.size() << " frames, loop="
              << (m_sequence.loop ? "true" : "false") << ")" << std::endl;
    return true;
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

bool AnimationController::PreloadVideo(const std::string& videoPath) {
    if (videoPath.empty()) return false;

    if (m_videoPlayers.find(videoPath) != m_videoPlayers.end()) {
        return true; // Already preloaded
    }

    std::wstring vpath = ResolveVideoPath(videoPath);
    if (vpath.empty()) {
        std::cerr << "[AnimationController] Could not resolve video path for preload: " << videoPath << std::endl;
        return false;
    }

    std::unique_ptr<VideoPlayer> player(new VideoPlayer());
    if (player->Preload(vpath, true)) {
        std::cout << "[AnimationController] Preloaded background video: " << videoPath << std::endl;
        m_videoPlayers[videoPath] = std::move(player);
        return true;
    }

    return false;
}

void AnimationController::PreloadMedia(RenderEngine& renderEngine) {
    // 1. Explicitly preloaded images
    for (const auto& img : m_sequence.preloadImages) {
        if (!img.empty()) {
            renderEngine.GetOrLoadImage(img);
        }
    }

    // 2. Images referenced across frames
    for (const auto& frame : m_sequence.frames) {
        for (const auto& action : frame.actions) {
            if ((action.type == ActionType::SetImage || action.type == ActionType::PreloadImage) &&
                !action.imagePath.empty()) {
                renderEngine.GetOrLoadImage(action.imagePath);
            }
        }
    }

    // 3. Explicitly preloaded videos
    for (const auto& vid : m_sequence.preloadVideos) {
        if (!vid.empty()) {
            PreloadVideo(vid);
        }
    }

    // 4. Videos referenced across frames
    for (const auto& frame : m_sequence.frames) {
        for (const auto& action : frame.actions) {
            if (action.type == ActionType::SetBackgroundVideo && !action.videoPath.empty()) {
                PreloadVideo(action.videoPath);
            }
        }
    }
}

void AnimationController::Play() {
    if (!m_sequence.Empty()) {
        m_isPlaying = true;
        if (m_activeVideoPlayer) {
            m_activeVideoPlayer->Play();
        }
    }
}

void AnimationController::Pause() {
    m_isPlaying = false;
    if (m_activeVideoPlayer) {
        m_activeVideoPlayer->Pause();
    }
}

void AnimationController::TogglePlayPause() {
    if (m_sequence.Empty()) return;
    m_isPlaying = !m_isPlaying;
    if (m_isPlaying) {
        if (m_activeVideoPlayer) m_activeVideoPlayer->Play();
    } else {
        if (m_activeVideoPlayer) m_activeVideoPlayer->Pause();
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
    if (m_activeVideoPlayer) {
        m_activeVideoPlayer->Stop();
        m_activeVideoPlayer = nullptr;
    }
    m_activePalpitations.clear();
}

void AnimationController::Restart(PatternGrid& grid) {
    if (m_sequence.Empty()) return;
    ClearPalpitations(grid);
    if (m_activeVideoPlayer) {
        m_activeVideoPlayer->Stop();
        m_activeVideoPlayer = nullptr;
    }
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
    if (m_activeVideoPlayer && m_activeVideoPlayer->IsPlaying()) {
        m_activeVideoPlayer->Update(deltaTime);
    }

    if (!m_activePalpitations.empty()) {
        UpdatePalpitations(deltaTime, grid);
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

void AnimationController::RemovePalpitationsForArea(int areaIndex, PatternGrid& grid) {
    for (auto it = m_activePalpitations.begin(); it != m_activePalpitations.end(); ) {
        auto& indices = it->areaIndices;
        indices.erase(std::remove(indices.begin(), indices.end(), areaIndex), indices.end());
        if (indices.empty()) {
            it = m_activePalpitations.erase(it);
        } else {
            ++it;
        }
    }
}

void AnimationController::ApplyFrame(const AnimationFrame& frame, PatternGrid& grid) {
    for (const auto& action : frame.actions) {
        switch (action.type) {
        case ActionType::AllOn:
            grid.SetAllVisible(true);
            ClearPalpitations(grid);
            grid.SetAllColor(RGB(255, 255, 255));
            break;

        case ActionType::AllOff:
            grid.SetAllVisible(false);
            ClearPalpitations(grid);
            break;

        case ActionType::TurnOn: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                RemovePalpitationsForArea(idx, grid);
                grid.SetAreaColor(idx, RGB(255, 255, 255));
                grid.SetAreaVisible(idx, true);
            }
            break;
        }

        case ActionType::TurnOff: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                RemovePalpitationsForArea(idx, grid);
                grid.SetAreaVisible(idx, false);
            }
            break;
        }

        case ActionType::Toggle: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                RemovePalpitationsForArea(idx, grid);
                grid.ToggleArea(idx);
            }
            break;
        }

        case ActionType::SetMask: {
            for (size_t i = 0; i < action.mask.size() && i < grid.GetCount(); ++i) {
                if (!action.mask[i]) {
                    RemovePalpitationsForArea(static_cast<int>(i), grid);
                }
                grid.SetAreaVisible(static_cast<int>(i), action.mask[i]);
            }
            break;
        }

        case ActionType::SetImage: {
            int idx = ResolveTargetIndex(action, grid);
            if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                RemovePalpitationsForArea(idx, grid);
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
                RemovePalpitationsForArea(idx, grid);
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
            break;
        }

        case ActionType::SetBackgroundVideo: {
            auto it = m_videoPlayers.find(action.videoPath);
            if (it == m_videoPlayers.end()) {
                if (PreloadVideo(action.videoPath)) {
                    it = m_videoPlayers.find(action.videoPath);
                }
            }

            if (it != m_videoPlayers.end()) {
                if (m_activeVideoPlayer && m_activeVideoPlayer != it->second.get()) {
                    m_activeVideoPlayer->Stop();
                }
                m_activeVideoPlayer = it->second.get();
                m_activeVideoPlayer->Play();
            } else {
                std::cerr << "[AnimationController] Could not find or open background video: "
                          << action.videoPath << std::endl;
            }
            break;
        }

        case ActionType::StopBackgroundVideo: {
            if (m_activeVideoPlayer) {
                m_activeVideoPlayer->Stop();
                m_activeVideoPlayer = nullptr;
            }
            break;
        }

        case ActionType::Palpitate: {
            ActivePalpitation palp;
            palp.minBrightness = action.minBrightness;
            palp.maxBrightness = action.maxBrightness;
            palp.frequency = action.frequency > 0.0f ? action.frequency : 1.2f;
            palp.timer = 0.0;

            if (action.hasCustomPhase) {
                palp.initialPhase = action.initialPhase;
            } else {
                float range = action.maxBrightness - action.minBrightness;
                if (range > 0.0001f) {
                    float fraction = (action.startBrightness - action.minBrightness) / range;
                    fraction = std::clamp(fraction, 0.0f, 1.0f);
                    // Standard cosine breathing wave: wave = 0.5 + 0.5 * cos(phase).
                    // Peak (1.0) is at phase 0, minimum (0.0) is at phase PI.
                    double baseAngle = std::acos(std::clamp(2.0 * fraction - 1.0, -1.0, 1.0));
                    if (action.startFalling) {
                        palp.initialPhase = baseAngle;
                    } else {
                        palp.initialPhase = 2.0 * 3.14159265358979323846 - baseAngle;
                    }
                } else {
                    palp.initialPhase = 0.0;
                }
            }

            if (action.targetId == -2) { // ALL
                ClearPalpitations(grid);
                for (size_t i = 0; i < grid.GetCount(); ++i) {
                    palp.areaIndices.push_back(static_cast<int>(i));
                }
            } else {
                for (int id : action.targetIds) {
                    int idx = grid.FindIndexById(id);
                    if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                        RemovePalpitationsForArea(idx, grid);
                        palp.areaIndices.push_back(idx);
                    }
                }
                if (!action.targetName.empty()) {
                    int idx = grid.FindIndexByName(action.targetName);
                    if (idx >= 0 && idx < static_cast<int>(grid.GetCount())) {
                        RemovePalpitationsForArea(idx, grid);
                        palp.areaIndices.push_back(idx);
                    }
                }
            }

            if (!palp.areaIndices.empty()) {
                double phase = palp.initialPhase;
                double wave = 0.5 + 0.5 * std::cos(phase);
                float brightness = palp.minBrightness + static_cast<float>(wave) * (palp.maxBrightness - palp.minBrightness);
                int val = static_cast<int>(std::clamp(brightness * 255.0f + 0.5f, 0.0f, 255.0f));
                COLORREF col = RGB(val, val, val);
                for (int idx : palp.areaIndices) {
                    grid.SetAreaColor(idx, col);
                    grid.SetAreaVisible(idx, true);
                }
                m_activePalpitations.push_back(palp);
            }
            break;
        }

        case ActionType::StopPalpitate: {
            ClearPalpitations(grid);
            break;
        }
        }
    }
}

void AnimationController::ClearPalpitations(PatternGrid& grid) {
    for (const auto& p : m_activePalpitations) {
        for (int idx : p.areaIndices) {
            grid.SetAreaColor(idx, RGB(255, 255, 255));
        }
    }
    m_activePalpitations.clear();
}

void AnimationController::UpdatePalpitations(double deltaTime, PatternGrid& grid) {
    for (auto& p : m_activePalpitations) {
        p.timer += deltaTime;

        // Smooth cosine ease-in-out breathing oscillation: peak is at 0, smooth descent to minimum at PI
        double phase = p.initialPhase + p.timer * p.frequency * 2.0 * 3.14159265358979323846;
        double wave = 0.5 + 0.5 * std::cos(phase);

        float brightness = p.minBrightness + static_cast<float>(wave) * (p.maxBrightness - p.minBrightness);
        int val = static_cast<int>(std::clamp(brightness * 255.0f + 0.5f, 0.0f, 255.0f));
        COLORREF color = RGB(val, val, val);

        for (int idx : p.areaIndices) {
            grid.SetAreaColor(idx, color);
            grid.SetAreaVisible(idx, true);
        }
    }
}
