#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <string>
#include <vector>

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // Video file control
    bool Open(const std::wstring& filePath, bool loop = true);
    void Close();

    void Play();
    void Pause();
    void Stop();

    // Advances playback according to elapsed time
    void Update(double deltaTime);

    // Frame access
    bool IsPlaying() const { return m_isPlaying; }
    bool HasFrame() const { return m_hasFrame; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    const BYTE* GetFrameData() const { return m_frameBuffer.data(); }

private:
    bool ReadNextFrame();

    IMFSourceReader* m_reader;
    bool m_mfInitialized;
    bool m_isPlaying;
    bool m_loop;
    bool m_hasFrame;

    int m_width;
    int m_height;
    double m_frameDuration;
    double m_timeAccumulator;

    std::vector<BYTE> m_frameBuffer;
};

#endif // VIDEO_PLAYER_H
