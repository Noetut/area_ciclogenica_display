#include "VideoPlayer.h"

#include <propvarutil.h>
#include <iostream>
#include <cstring>

#include "util/StringUtil.h"

VideoPlayer::VideoPlayer()
    : m_reader(NULL)
    , m_mfInitialized(false)
    , m_isPlaying(false)
    , m_loop(true)
    , m_hasFrame(false)
    , m_width(0)
    , m_height(0)
    , m_frameDuration(1.0 / 30.0)
    , m_timeAccumulator(0.0)
{
    Initialize();
}

VideoPlayer::~VideoPlayer() {
    Close();
    Shutdown();
}

bool VideoPlayer::Initialize() {
    if (m_mfInitialized) return true;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    HRESULT hr = MFStartup(MF_VERSION);
    if (SUCCEEDED(hr)) {
        m_mfInitialized = true;
        return true;
    }
    std::cerr << "[VideoPlayer] MFStartup failed: " << std::hex << hr << std::endl;
    return false;
}

void VideoPlayer::Shutdown() {
    if (m_mfInitialized) {
        MFShutdown();
        CoUninitialize();
        m_mfInitialized = false;
    }
}

bool VideoPlayer::Open(const std::wstring& filePath, bool loop) {
    Close();

    if (!m_mfInitialized && !Initialize()) {
        return false;
    }

    m_loop = loop;

    IMFAttributes* pAttributes = NULL;
    MFCreateAttributes(&pAttributes, 1);
    if (pAttributes) {
        pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    }

    HRESULT hr = MFCreateSourceReaderFromURL(filePath.c_str(), pAttributes, &m_reader);
    if (pAttributes) {
        pAttributes->Release();
    }

    if (FAILED(hr) || !m_reader) {
        std::cerr << "[VideoPlayer] Could not open video file: " << WideToUtf8(filePath) << std::endl;
        return false;
    }

    // Configure video stream to RGB32 (32-bit DIB BGRA)
    IMFMediaType* pMediaType = NULL;
    MFCreateMediaType(&pMediaType);
    pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

    hr = m_reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pMediaType);
    pMediaType->Release();

    if (FAILED(hr)) {
        std::cerr << "[VideoPlayer] Could not configure RGB32 output for video." << std::endl;
        Close();
        return false;
    }

    IMFMediaType* pCurrentType = NULL;
    if (SUCCEEDED(m_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType))) {
        UINT32 w = 0, h = 0;
        MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &w, &h);
        m_width = static_cast<int>(w);
        m_height = static_cast<int>(h);

        UINT32 num = 0, den = 0;
        if (SUCCEEDED(MFGetAttributeRatio(pCurrentType, MF_MT_FRAME_RATE, &num, &den)) && den > 0 && num > 0) {
            m_frameDuration = static_cast<double>(den) / static_cast<double>(num);
        } else {
            m_frameDuration = 1.0 / 30.0;
        }

        pCurrentType->Release();
    }

    if (m_width <= 0 || m_height <= 0) {
        std::cerr << "[VideoPlayer] Invalid video dimensions." << std::endl;
        Close();
        return false;
    }

    size_t bufferSize = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
    m_frameBuffer.resize(bufferSize, 0);

    // Read first frame immediately
    ReadNextFrame();

    m_isPlaying = true;
    m_timeAccumulator = 0.0;

    std::cout << "[VideoPlayer] Opened video: " << WideToUtf8(filePath)
              << " (" << m_width << "x" << m_height << " @ " << (1.0 / m_frameDuration) << " fps)" << std::endl;
    return true;
}

void VideoPlayer::Close() {
    m_isPlaying = false;
    m_hasFrame = false;
    m_width = 0;
    m_height = 0;
    m_timeAccumulator = 0.0;
    m_frameBuffer.clear();

    if (m_reader) {
        m_reader->Release();
        m_reader = NULL;
    }
}

void VideoPlayer::Play() {
    if (m_reader) {
        m_isPlaying = true;
    }
}

void VideoPlayer::Pause() {
    m_isPlaying = false;
}

void VideoPlayer::Stop() {
    m_isPlaying = false;
    if (m_reader) {
        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = 0;
        m_reader->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);
    }
    m_timeAccumulator = 0.0;
}

void VideoPlayer::Update(double deltaTime) {
    if (!m_isPlaying || !m_reader) return;

    m_timeAccumulator += deltaTime;

    // Advance frames if enough time has passed
    if (m_timeAccumulator >= m_frameDuration) {
        ReadNextFrame();
        m_timeAccumulator -= m_frameDuration;
        // Cap accumulator to avoid spiral of death on long frames
        if (m_timeAccumulator > m_frameDuration * 2.0) {
            m_timeAccumulator = 0.0;
        }
    }
}

bool VideoPlayer::ReadNextFrame() {
    if (!m_reader) return false;

    DWORD streamIndex = 0, flags = 0;
    LONGLONG timestamp = 0;
    IMFSample* pSample = NULL;

    HRESULT hr = m_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
                                      &streamIndex, &flags, &timestamp, &pSample);
    if (FAILED(hr)) {
        return false;
    }

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
        if (m_loop) {
            PROPVARIANT var;
            PropVariantInit(&var);
            var.vt = VT_I8;
            var.hVal.QuadPart = 0;
            m_reader->SetCurrentPosition(GUID_NULL, var);
            PropVariantClear(&var);

            // Read first frame of new cycle
            return ReadNextFrame();
        }
        m_isPlaying = false;
        return false;
    }

    if (pSample) {
        IMFMediaBuffer* pBuffer = NULL;
        if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer))) {
            BYTE* pData = NULL;
            DWORD maxLen = 0, curLen = 0;
            if (SUCCEEDED(pBuffer->Lock(&pData, &maxLen, &curLen))) {
                size_t expectedSize = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
                if (m_frameBuffer.size() != expectedSize) {
                    m_frameBuffer.resize(expectedSize);
                }
                size_t copyBytes = (curLen < expectedSize) ? curLen : expectedSize;
                std::memcpy(m_frameBuffer.data(), pData, copyBytes);
                m_hasFrame = true;
                pBuffer->Unlock();
            }
            pBuffer->Release();
        }
        pSample->Release();
        return true;
    }

    return false;
}
