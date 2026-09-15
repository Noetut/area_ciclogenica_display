#include "VideoPlayer.h"

#include <propvarutil.h>
#include <iostream>
#include <cstring>
#include <algorithm>

#include "util/StringUtil.h"

VideoPlayer::VideoPlayer()
    : m_reader(NULL)
    , m_mfInitialized(false)
    , m_loop(true)
    , m_width(0)
    , m_height(0)
    , m_frameDuration(1.0 / 30.0)
    , m_timeAccumulator(0.0)
    , m_isPlaying(false)
    , m_hasFrame(false)
    , m_stopWorker(false)
    , m_workerActive(false)
{
    Initialize();
}

VideoPlayer::~VideoPlayer() {
    Close();
    Shutdown();
}

bool VideoPlayer::Initialize() {
    if (m_mfInitialized) return true;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT hr = MFStartup(MF_VERSION);
    if (SUCCEEDED(hr)) {
        m_mfInitialized = true;
        return true;
    }
    std::cerr << "[VideoPlayer] MFStartup failed: " << std::hex << hr << std::endl;
    return false;
}

void VideoPlayer::Shutdown() {
    Close();
    if (m_mfInitialized) {
        MFShutdown();
        CoUninitialize();
        m_mfInitialized = false;
    }
}

bool VideoPlayer::SetupSourceReader(const std::wstring& filePath) {
    if (!m_mfInitialized && !Initialize()) {
        return false;
    }

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
        m_reader->Release();
        m_reader = NULL;
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
        m_reader->Release();
        m_reader = NULL;
        return false;
    }

    m_currentFilePath = filePath;
    size_t bufferSize = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
    {
        std::lock_guard<std::mutex> lock(m_displayMutex);
        m_displayBuffer.resize(bufferSize, 0);
    }

    // Pre-allocate buffer pool to eliminate runtime allocations
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_freeBuffers.clear();
        for (size_t i = 0; i < kMaxQueueSize + 2; ++i) {
            m_freeBuffers.emplace_back(bufferSize, 0);
        }
    }

    // Read first frame immediately so display buffer is populated
    std::vector<BYTE> initialFrame(bufferSize, 0);
    if (DecodeNextSample(initialFrame)) {
        std::lock_guard<std::mutex> lock(m_displayMutex);
        m_displayBuffer = std::move(initialFrame);
        m_hasFrame = true;
    }

    return true;
}

bool VideoPlayer::Preload(const std::wstring& filePath, bool loop) {
    if (m_reader && m_currentFilePath == filePath) {
        m_loop = loop;
        return true;
    }

    Close();
    m_loop = loop;

    if (!SetupSourceReader(filePath)) {
        return false;
    }

    StartWorker();

    std::cout << "[VideoPlayer] Preloaded video: " << WideToUtf8(filePath)
              << " (" << m_width << "x" << m_height << " @ " << (1.0 / m_frameDuration) << " fps)" << std::endl;
    return true;
}

bool VideoPlayer::Open(const std::wstring& filePath, bool loop) {
    if (m_reader && m_currentFilePath == filePath) {
        m_loop = loop;
        Play();
        return true;
    }

    if (!Preload(filePath, loop)) {
        return false;
    }

    Play();
    return true;
}

void VideoPlayer::Close() {
    StopWorker();

    m_isPlaying = false;
    m_hasFrame = false;
    m_width = 0;
    m_height = 0;
    m_timeAccumulator = 0.0;
    m_currentFilePath.clear();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_frameQueue.clear();
        m_freeBuffers.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_displayMutex);
        m_displayBuffer.clear();
    }

    if (m_reader) {
        m_reader->Release();
        m_reader = NULL;
    }
}

void VideoPlayer::StartWorker() {
    StopWorker();
    m_stopWorker = false;
    m_workerActive = false;
    m_workerThread = std::thread(&VideoPlayer::WorkerLoop, this);
}

void VideoPlayer::StopWorker() {
    m_stopWorker = true;
    m_workerActive = false;
    m_cvWorker.notify_all();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void VideoPlayer::Play() {
    if (m_reader) {
        m_isPlaying = true;
        m_workerActive = true;
        m_cvWorker.notify_all();
    }
}

void VideoPlayer::Pause() {
    m_isPlaying = false;
    m_workerActive = false;
}

void VideoPlayer::Stop() {
    m_isPlaying = false;
    m_workerActive = false;
    m_timeAccumulator = 0.0;

    if (m_reader) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        // Drain queue
        while (!m_frameQueue.empty()) {
            m_freeBuffers.push_back(std::move(m_frameQueue.front()));
            m_frameQueue.pop_front();
        }

        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = 0;
        m_reader->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);

        // Decode first frame into display buffer
        size_t expectedSize = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
        std::vector<BYTE> firstFrame(expectedSize, 0);
        if (DecodeNextSample(firstFrame)) {
            std::lock_guard<std::mutex> dLock(m_displayMutex);
            m_displayBuffer = std::move(firstFrame);
            m_hasFrame = true;
        }
    }
}

const BYTE* VideoPlayer::GetFrameData() const {
    std::lock_guard<std::mutex> lock(m_displayMutex);
    return m_displayBuffer.empty() ? nullptr : m_displayBuffer.data();
}

void VideoPlayer::Update(double deltaTime) {
    if (!m_isPlaying || !m_reader) return;

    m_timeAccumulator += deltaTime;

    while (m_timeAccumulator >= m_frameDuration) {
        m_timeAccumulator -= m_frameDuration;

        std::vector<BYTE> nextFrame;
        bool framePopped = false;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (!m_frameQueue.empty()) {
                nextFrame = std::move(m_frameQueue.front());
                m_frameQueue.pop_front();
                framePopped = true;
                m_cvWorker.notify_one();
            }
        }

        if (framePopped) {
            std::lock_guard<std::mutex> dLock(m_displayMutex);
            if (!nextFrame.empty()) {
                m_displayBuffer = std::move(nextFrame);
                m_hasFrame = true;
            }
        }
    }

    if (m_timeAccumulator > m_frameDuration * 2.0) {
        m_timeAccumulator = 0.0;
    }
}

bool VideoPlayer::DecodeNextSample(std::vector<BYTE>& outBuffer) {
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

            return DecodeNextSample(outBuffer);
        }
        m_isPlaying = false;
        m_workerActive = false;
        return false;
    }

    if (pSample) {
        IMFMediaBuffer* pBuffer = NULL;
        if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer))) {
            BYTE* pData = NULL;
            DWORD maxLen = 0, curLen = 0;
            if (SUCCEEDED(pBuffer->Lock(&pData, &maxLen, &curLen))) {
                size_t expectedSize = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
                if (outBuffer.size() != expectedSize) {
                    outBuffer.resize(expectedSize);
                }
                size_t copyBytes = (curLen < expectedSize) ? curLen : expectedSize;
                std::memcpy(outBuffer.data(), pData, copyBytes);
                pBuffer->Unlock();
            }
            pBuffer->Release();
        }
        pSample->Release();
        return true;
    }

    return false;
}

void VideoPlayer::WorkerLoop() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    while (!m_stopWorker) {
        std::vector<BYTE> buffer;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_cvWorker.wait(lock, [this]() {
                return m_stopWorker || (m_workerActive && m_frameQueue.size() < kMaxQueueSize);
            });

            if (m_stopWorker) break;

            if (!m_freeBuffers.empty()) {
                buffer = std::move(m_freeBuffers.back());
                m_freeBuffers.pop_back();
            } else {
                size_t expectedSize = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
                buffer.resize(expectedSize, 0);
            }
        }

        if (DecodeNextSample(buffer)) {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_frameQueue.push_back(std::move(buffer));
        } else {
            // If decode failed or reached non-looping EOF, return buffer to free pool
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_freeBuffers.push_back(std::move(buffer));
            // Brief sleep to prevent tight loop on error/EOF
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    CoUninitialize();
}
