#include "Application.h"
#include <iostream>
#include <thread>

Application::Application()
    : m_isRunning(false)
    , m_targetMonitorIndex(1)
    , m_isFullBlinkActive(false)
    , m_blinkTimer(0.0)
    , m_blinkInterval(1.0)
    , m_allSquaresOn(true)
{
}

Application::~Application() {
}

bool Application::Initialize(int targetMonitorIndex, bool enableBlink, double blinkInterval) {
    m_targetMonitorIndex = targetMonitorIndex;
    m_isFullBlinkActive = enableBlink;
    m_blinkInterval = (blinkInterval > 0.0) ? blinkInterval : 1.0;
    m_blinkTimer = 0.0;

    std::cout << "[Application] Initializing Patron Animation App..." << std::endl;

    // Step 1: Create full-screen window on target monitor (Monitor 2 / index 1)
    HWND hwnd = m_displayManager.CreateWindowOnMonitor(m_targetMonitorIndex, L"Patrón Animation - Grid Display");
    if (!hwnd) {
        std::cerr << "[Application] ERROR: DisplayManager failed to create window." << std::endl;
        return false;
    }

    // Step 2: Initialize double-buffered GDI rendering engine
    if (!m_renderEngine.Initialize(hwnd, m_displayManager.GetWidth(), m_displayManager.GetHeight())) {
        std::cerr << "[Application] ERROR: RenderEngine failed to initialize." << std::endl;
        return false;
    }

    // Step 3: Initialize pattern grid with all squares ON initially
    m_grid.ResetToDefaults();
    m_allSquaresOn = true;
    m_grid.SetAllVisible(m_allSquaresOn);

    std::cout << "[Application] Initialization complete! Target: Monitor #" << m_targetMonitorIndex << std::endl;
    std::cout << "[Application] Total pattern squares loaded: " << m_grid.GetCount() << std::endl;
    std::cout << "---------------------------------------------------------" << std::endl;
    std::cout << "                      CONTROLS                           " << std::endl;
    std::cout << "  [1 - 9] Toggle individual square ON/OFF                  " << std::endl;
    std::cout << "  [A]     Turn ALL squares ON                             " << std::endl;
    std::cout << "  [O]     Turn ALL squares OFF (Clear to black)           " << std::endl;
    std::cout << "  [ESC]   Exit application                                " << std::endl;
    std::cout << "---------------------------------------------------------" << std::endl;
    if (m_isFullBlinkActive) {
        std::cout << "[Application] Blink mode: ACTIVE (" << m_blinkInterval << "s interval)" << std::endl;
    } else {
        std::cout << "[Application] Blink mode: OFF (Static display)" << std::endl;
    }

    m_isRunning = true;
    return true;
}

void Application::SetSquareVisible(int id, bool visible) {
    m_grid.SetSquareVisible(id, visible);
}

void Application::SetSquareVisible(BoxId id, bool visible) {
    m_grid.SetSquareVisible(id, visible);
}

void Application::ToggleSquare(int id) {
    m_grid.ToggleSquare(id);
}

void Application::ToggleSquare(BoxId id) {
    m_grid.ToggleSquare(id);
}

void Application::SetAllSquaresVisible(bool visible) {
    m_grid.SetAllVisible(visible);
    m_allSquaresOn = visible;
}

void Application::SetFullBlinkActive(bool active) {
    m_isFullBlinkActive = active;
    m_blinkTimer = 0.0;
}

void Application::HandleKeyDown(WPARAM key) {
    if (key == VK_ESCAPE) {
        m_isRunning = false;
        return;
    }

    if (key >= '1' && key <= '9') {
        int id = static_cast<int>(key - '1');
        m_grid.ToggleSquare(id);
        const auto* sq = m_grid.GetSquare(id);
        if (sq) {
            std::cout << "[Application] Square #" << id << " [" << sq->name << "] toggled -> " 
                      << (sq->isVisible ? "ON" : "OFF") << std::endl;
        }
        return;
    }

    if (key == 'A' || key == 'a') {
        SetAllSquaresVisible(true);
        std::cout << "[Application] All squares turned ON." << std::endl;
        return;
    }

    if (key == 'O' || key == 'o') {
        SetAllSquaresVisible(false);
        std::cout << "[Application] All squares turned OFF (Clear)." << std::endl;
        return;
    }
}

void Application::ProcessEvents() {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_isRunning = false;
        } else if (msg.message == WM_KEYDOWN) {
            HandleKeyDown(msg.wParam);
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Application::Update(double deltaTime) {
    // 1-second full blink loop: alternate all squares between ON and OFF every 1.0s
    if (m_isFullBlinkActive) {
        m_blinkTimer += deltaTime;
        if (m_blinkTimer >= m_blinkInterval) {
            m_blinkTimer -= m_blinkInterval;
            m_allSquaresOn = !m_allSquaresOn;
            m_grid.SetAllVisible(m_allSquaresOn);

            std::cout << "[Application] Full Blink: All squares -> " 
                      << (m_allSquaresOn ? "ON (White)" : "OFF (Black)") << std::endl;
        }
    }
}

void Application::Render() {
    m_renderEngine.BeginFrame();
    m_renderEngine.RenderSquares(m_grid.GetSquares());
    m_renderEngine.EndFrame();
}

void Application::Run() {
    using clock = std::chrono::high_resolution_clock;
    auto previousTime = clock::now();

    const double targetFPS = 60.0;
    const std::chrono::duration<double> targetFrameDuration(1.0 / targetFPS);

    while (m_isRunning) {
        auto currentTime = clock::now();
        std::chrono::duration<double> elapsedTime = currentTime - previousTime;
        previousTime = currentTime;

        double deltaTime = elapsedTime.count();

        ProcessEvents();
        Update(deltaTime);
        Render();

        // Cap frame rate at 60 FPS to prevent CPU hogging
        auto frameEndTime = clock::now();
        auto frameDuration = frameEndTime - currentTime;
        if (frameDuration < targetFrameDuration) {
            std::this_thread::sleep_for(targetFrameDuration - frameDuration);
        }
    }

    std::cout << "[Application] Exiting cleanly." << std::endl;
}

void Application::Quit() {
    m_isRunning = false;
}
