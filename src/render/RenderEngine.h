#ifndef RENDER_ENGINE_H
#define RENDER_ENGINE_H

#include <windows.h>
#include <vector>
#include "model/PatternBoxes.h"

class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    // Initialize rendering resources for window handle and dimensions
    bool Initialize(HWND hwnd, int width, int height);

    // Clean up GDI memory DCs and buffers
    void Cleanup();

    // Resize buffers if window size changes
    void Resize(int width, int height);

    // Begin frame rendering
    void BeginFrame();

    // Render solid black image frame
    void RenderBlack();

    // Render all active squares based on their visibility and color
    void RenderSquares(const std::vector<SquareData>& squares);

    // Present offscreen frame buffer to target window
    void EndFrame();

    // Get backbuffer DC for future component/animation rendering
    HDC GetBackBufferDC() const { return m_memDC; }

private:
    HWND m_hwnd;
    HDC m_hdc;
    HDC m_memDC;
    HBITMAP m_hBitmap;
    HBITMAP m_hOldBitmap;
    HBRUSH m_blackBrush;
    HBRUSH m_whiteBrush;
    int m_width;
    int m_height;
};

#endif // RENDER_ENGINE_H
