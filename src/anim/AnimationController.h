#ifndef ANIMATION_CONTROLLER_H
#define ANIMATION_CONTROLLER_H

#include <string>
#include "AnimationTypes.h"

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

private:
    void ApplyFrame(const AnimationFrame& frame, PatternGrid& grid);

    AnimationSequence m_sequence;
    std::string       m_filePath;
    size_t            m_currentFrameIndex;
    double            m_frameTimer;
    bool              m_isPlaying;
    bool              m_isWaitingForClick;
    bool              m_frameApplied;
};

#endif // ANIMATION_CONTROLLER_H
