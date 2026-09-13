#include "RenderEngine.h"
#include <iostream>

RenderEngine::RenderEngine()
    : m_hwnd(NULL)
    , m_hdc(NULL)
    , m_memDC(NULL)
    , m_hBitmap(NULL)
    , m_hOldBitmap(NULL)
    , m_blackBrush(NULL)
    , m_whiteBrush(NULL)
    , m_width(0)
    , m_height(0)
{
}

RenderEngine::~RenderEngine() {
    Cleanup();
}

bool RenderEngine::Initialize(HWND hwnd, int width, int height) {
    Cleanup();

    m_hwnd = hwnd;
    m_width = width;
    m_height = height;

    m_hdc = GetDC(m_hwnd);
    if (!m_hdc) {
        std::cerr << "[RenderEngine] ERROR: Could not get device context." << std::endl;
        return false;
    }

    m_memDC = CreateCompatibleDC(m_hdc);
    m_hBitmap = CreateCompatibleBitmap(m_hdc, m_width, m_height);
    m_hOldBitmap = (HBITMAP)SelectObject(m_memDC, m_hBitmap);

    // Cache common brushes for high-performance rendering without leaks
    m_blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    m_whiteBrush = CreateSolidBrush(RGB(255, 255, 255));

    std::cout << "[RenderEngine] Initialized offscreen double buffer (" 
              << m_width << "x" << m_height << ")" << std::endl;
    return true;
}

void RenderEngine::Cleanup() {
    if (m_blackBrush) {
        DeleteObject(m_blackBrush);
        m_blackBrush = NULL;
    }

    if (m_whiteBrush) {
        DeleteObject(m_whiteBrush);
        m_whiteBrush = NULL;
    }

    if (m_memDC) {
        if (m_hOldBitmap) {
            SelectObject(m_memDC, m_hOldBitmap);
            m_hOldBitmap = NULL;
        }
        DeleteDC(m_memDC);
        m_memDC = NULL;
    }

    if (m_hBitmap) {
        DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }

    if (m_hdc && m_hwnd) {
        ReleaseDC(m_hwnd, m_hdc);
        m_hdc = NULL;
    }
}

void RenderEngine::Resize(int width, int height) {
    if (width != m_width || height != m_height) {
        Initialize(m_hwnd, width, height);
    }
}

void RenderEngine::BeginFrame() {
    // Ready memory DC for drawing
}

void RenderEngine::RenderBlack() {
    if (!m_memDC) return;

    RECT rect = { 0, 0, m_width, m_height };
    FillRect(m_memDC, &rect, m_blackBrush ? m_blackBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));
}

void RenderEngine::RenderSquares(const std::vector<SquareData>& squares) {
    if (!m_memDC) return;

    // Step 1: Clear the entire frame to solid black
    RenderBlack();

    // Step 2: Render each visible square with its assigned color
    for (const auto& sq : squares) {
        if (!sq.isVisible) continue;

        HBRUSH brushToUse = NULL;
        bool customBrush = false;

        if (sq.color == RGB(255, 255, 255) && m_whiteBrush) {
            brushToUse = m_whiteBrush;
        } else {
            brushToUse = CreateSolidBrush(sq.color);
            customBrush = true;
        }

        FillRect(m_memDC, &sq.rect, brushToUse);

        if (customBrush) {
            DeleteObject(brushToUse);
        }
    }
}

void RenderEngine::EndFrame() {
    if (!m_hdc || !m_memDC) return;

    // Fast bit-block transfer from offscreen buffer to window DC
    BitBlt(m_hdc, 0, 0, m_width, m_height, m_memDC, 0, 0, SRCCOPY);
}
