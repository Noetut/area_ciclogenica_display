#ifndef APPLICATION_H
#define APPLICATION_H

#include "../display/DisplayManager.h"
#include "../render/RenderEngine.h"
#include "../model/PatternGrid.h"
#include <chrono>

class Application {
public:
    Application();
    ~Application();

    // Initialize application targeting display index (default 1 for 2nd monitor)
    bool Initialize(int targetMonitorIndex = 1);

    // Main application engine loop
    void Run();

    // Stop and exit application
    void Quit();

    // Square control functions
    void SetSquareVisible(int id, bool visible);
    void SetSquareVisible(BoxId id, bool visible);
    void ToggleSquare(int id);
    void ToggleSquare(BoxId id);
    void SetAllSquaresVisible(bool visible);

    // Full blink mode control
    void SetFullBlinkActive(bool active);
    bool IsFullBlinkActive() const { return m_isFullBlinkActive; }
    void SetBlinkInterval(double seconds) { m_blinkInterval = seconds; }

    // Pattern grid accessor
    PatternGrid& GetGrid() { return m_grid; }
    const PatternGrid& GetGrid() const { return m_grid; }

private:
    void ProcessEvents();
    void HandleKeyDown(WPARAM key);
    void Update(double deltaTime);
    void Render();

    DisplayManager m_displayManager;
    RenderEngine m_renderEngine;
    PatternGrid m_grid;

    bool m_isRunning;
    int m_targetMonitorIndex;

    // Full blink loop state
    bool m_isFullBlinkActive;
    double m_blinkTimer;
    double m_blinkInterval; // 1.0 second per phase (1s ON, 1s OFF)
    bool m_allSquaresOn;
};

#endif // APPLICATION_H
