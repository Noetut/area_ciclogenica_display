#ifndef ANIMATION_CONTROLLER_H
#define ANIMATION_CONTROLLER_H

#include <string>
#include "AnimationTypes.h"
#include "media/VideoPlayer.h"

class PatternGrid;
class RenderEngine;

class AnimationController {
public:
    AnimationController();
    ~AnimationController();

    bool LoadFromFile(const std::string& filePath, std::string& outError);
    bool LoadFromSequence(const AnimationSequence& sequence);

    void PreloadImages(RenderEngine& renderEngine);

    void Play();
    void Pause();
    void TogglePlayPause();
    void Stop();
    void Restart(PatternGrid& grid);

    void TriggerClick(PatternGrid& grid);

    void Update(double deltaTime, PatternGrid& grid);

    bool IsPlaying() const { return m_isPlaying; }
    bool IsWaitingForClick() const { return m_isWaitingForClick; }
    bool HasSequence() const { return !m_sequence.Empty(); }
    size_t CurrentFrameIndex() const { return m_currentFrameIndex; }
    size_t TotalFrames() const { return m_sequence.FrameCount(); }
    const std::string& SequenceName() const { return m_sequence.name; }
    const std::string& FilePath() const { return m_filePath; }

    VideoPlayer& GetVideoPlayer() { return m_videoPlayer; }
    const VideoPlayer& GetVideoPlayer() const { return m_videoPlayer; }
    bool HasBackgroundVideo() const { return m_videoPlayer.IsPlaying() && m_videoPlayer.HasFrame(); }
    const BYTE* GetBackgroundVideoFrame(int& outW, int& outH) const {
        if (!m_videoPlayer.HasFrame() || !m_videoPlayer.IsPlaying()) return nullptr;
        outW = m_videoPlayer.GetWidth();
        outH = m_videoPlayer.GetHeight();
        return m_videoPlayer.GetFrameData();
    }

private:
    void ApplyFrame(const AnimationFrame& frame, PatternGrid& grid);

    VideoPlayer       m_videoPlayer;
    AnimationSequence m_sequence;
    std::string       m_filePath;
    size_t            m_currentFrameIndex;
    double            m_frameTimer;
    bool              m_isPlaying;
    bool              m_isWaitingForClick;
    bool              m_frameApplied;
};

#endif // ANIMATION_CONTROLLER_H
