#include "RenderEngine.h"

#include <iostream>

namespace {

// 8x8 monochrome checkerboard. With a monochrome pattern brush GDI paints the
// 0 bits in the DC text colour and the 1 bits in the DC background colour,
// which is what gives the selected area its half-lit look.
const WORD kHalftoneBits[8] = {
    0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555
};
const int kHalftoneSize = 8;   // Pixels per side of the pattern bitmap

// HUD font. Fixed pitch keeps the coordinate columns in the panel aligned; the
// negative height asks for a character height rather than a cell height.
const int      kHudFontHeight = -14;
const wchar_t* kHudFontFace   = L"Consolas";

// Black margin painted around HUD text so it stays readable over a lit area.
const int kHudTextPadding = 6;

} // namespace

RenderEngine::RenderEngine()
    : m_hwnd(NULL)
    , m_hdc(NULL)
    , m_memDC(NULL)
    , m_hBitmap(NULL)
    , m_hOldBitmap(NULL)
    , m_blackBrush(NULL)
    , m_whiteBrush(NULL)
    , m_halftoneBrush(NULL)
    , m_hudFont(NULL)
    , m_width(0)
    , m_height(0)
{
}

RenderEngine::~RenderEngine() {
    Cleanup();
}

// ---------------------------------------------------------------------------
// Resource lifecycle
// ---------------------------------------------------------------------------

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

    // Cache common GDI objects for high-performance rendering without leaks
    m_blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    m_whiteBrush = CreateSolidBrush(RGB(255, 255, 255));

    HBITMAP halftoneBitmap = CreateBitmap(kHalftoneSize, kHalftoneSize, 1, 1, kHalftoneBits);
    if (halftoneBitmap) {
        m_halftoneBrush = CreatePatternBrush(halftoneBitmap);
        DeleteObject(halftoneBitmap);
    }

    m_hudFont = CreateFontW(kHudFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, kHudFontFace);

    std::cout << "[RenderEngine] Initialized offscreen double buffer ("
              << m_width << "x" << m_height << ")" << std::endl;
    return true;
}

void RenderEngine::Cleanup() {
    for (auto& cached : m_pens) {
        if (cached.pen) DeleteObject(cached.pen);
    }
    m_pens.clear();

    if (m_hudFont) {
        DeleteObject(m_hudFont);
        m_hudFont = NULL;
    }

    if (m_halftoneBrush) {
        DeleteObject(m_halftoneBrush);
        m_halftoneBrush = NULL;
    }

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

// ---------------------------------------------------------------------------
// Frame and show-mode output
// ---------------------------------------------------------------------------

void RenderEngine::BeginFrame() {
    // Ready memory DC for drawing
}

void RenderEngine::EndFrame() {
    if (!m_hdc || !m_memDC) return;

    // Fast bit-block transfer from offscreen buffer to window DC
    BitBlt(m_hdc, 0, 0, m_width, m_height, m_memDC, 0, 0, SRCCOPY);
}

void RenderEngine::RenderBlack() {
    if (!m_memDC) return;

    RECT rect = { 0, 0, m_width, m_height };
    FillRect(m_memDC, &rect, m_blackBrush ? m_blackBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));
}

void RenderEngine::RenderAreas(const std::vector<ProjectionArea>& areas) {
    if (!m_memDC) return;

    RenderBlack();

    for (const auto& area : areas) {
        if (!area.isVisible) continue;
        FillQuad(area.quad, area.color);
    }
}

// ---------------------------------------------------------------------------
// Drawing primitives (used by the calibration overlay)
// ---------------------------------------------------------------------------

HPEN RenderEngine::GetPen(COLORREF color, int thickness) {
    if (thickness < 1) thickness = 1;

    for (const auto& cached : m_pens) {
        if (cached.color == color && cached.thickness == thickness) return cached.pen;
    }

    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    if (!pen) return (HPEN)GetStockObject(WHITE_PEN);

    CachedPen entry = { color, thickness, pen };
    m_pens.push_back(entry);
    return pen;
}

void RenderEngine::FillQuadWithBrush(const Quad& quad, HBRUSH brush) {
    POINT points[4];
    for (int i = 0; i < 4; ++i) {
        points[i].x = quad.corners[i].x;
        points[i].y = quad.corners[i].y;
    }

    HGDIOBJ oldBrush = SelectObject(m_memDC, brush);
    HGDIOBJ oldPen   = SelectObject(m_memDC, GetStockObject(NULL_PEN));
    Polygon(m_memDC, points, 4);
    SelectObject(m_memDC, oldPen);
    SelectObject(m_memDC, oldBrush);
}

void RenderEngine::FillQuad(const Quad& quad, COLORREF color) {
    if (!m_memDC) return;

    if (quad.IsAxisAlignedRect()) {
        RECT rect = quad.BoundingBox();
        if (color == RGB(255, 255, 255) && m_whiteBrush) {
            FillRect(m_memDC, &rect, m_whiteBrush);
        } else {
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(m_memDC, &rect, brush);
            DeleteObject(brush);
        }
        return;
    }

    if (color == RGB(255, 255, 255) && m_whiteBrush) {
        FillQuadWithBrush(quad, m_whiteBrush);
    } else {
        HBRUSH brush = CreateSolidBrush(color);
        FillQuadWithBrush(quad, brush);
        DeleteObject(brush);
    }
}

void RenderEngine::FillQuadHalftone(const Quad& quad, COLORREF color) {
    if (!m_memDC || !m_halftoneBrush) return;

    COLORREF oldText = SetTextColor(m_memDC, color);
    COLORREF oldBk   = SetBkColor(m_memDC, RGB(0, 0, 0));

    if (quad.IsAxisAlignedRect()) {
        RECT rect = quad.BoundingBox();
        FillRect(m_memDC, &rect, m_halftoneBrush);
    } else {
        FillQuadWithBrush(quad, m_halftoneBrush);
    }

    SetTextColor(m_memDC, oldText);
    SetBkColor(m_memDC, oldBk);
}

void RenderEngine::DrawQuadOutline(const Quad& quad, COLORREF color, int thickness) {
    if (!m_memDC) return;

    POINT points[5];
    for (int i = 0; i < 4; ++i) {
        points[i].x = quad.corners[i].x;
        points[i].y = quad.corners[i].y;
    }
    points[4] = points[0]; // Close the loop

    HGDIOBJ oldPen = SelectObject(m_memDC, GetPen(color, thickness));
    Polyline(m_memDC, points, 5);
    SelectObject(m_memDC, oldPen);
}

void RenderEngine::DrawHandle(const Point2i& point, int halfSize, COLORREF fill, COLORREF border) {
    if (!m_memDC) return;

    RECT rect = { point.x - halfSize, point.y - halfSize,
                  point.x + halfSize + 1, point.y + halfSize + 1 };

    HBRUSH brush = CreateSolidBrush(fill);
    FillRect(m_memDC, &rect, brush);
    DeleteObject(brush);

    HGDIOBJ oldPen   = SelectObject(m_memDC, GetPen(border, 1));
    HGDIOBJ oldBrush = SelectObject(m_memDC, GetStockObject(NULL_BRUSH));
    Rectangle(m_memDC, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(m_memDC, oldBrush);
    SelectObject(m_memDC, oldPen);
}

void RenderEngine::DrawCross(const Point2i& point, int armLength, COLORREF color) {
    if (!m_memDC) return;

    HGDIOBJ oldPen = SelectObject(m_memDC, GetPen(color, 1));
    MoveToEx(m_memDC, point.x - armLength, point.y, NULL);
    LineTo(m_memDC, point.x + armLength + 1, point.y);
    MoveToEx(m_memDC, point.x, point.y - armLength, NULL);
    LineTo(m_memDC, point.x, point.y + armLength + 1);
    SelectObject(m_memDC, oldPen);
}

void RenderEngine::DrawHudText(int x, int y, const std::wstring& text, COLORREF color) {
    if (!m_memDC || text.empty()) return;

    HGDIOBJ oldFont = m_hudFont ? SelectObject(m_memDC, m_hudFont) : NULL;

    const UINT format = DT_LEFT | DT_TOP | DT_NOPREFIX | DT_EXPANDTABS;

    RECT measured = { 0, 0, 0, 0 };
    DrawTextW(m_memDC, text.c_str(), static_cast<int>(text.size()), &measured,
              format | DT_CALCRECT);

    RECT backing = { x, y,
                     x + (measured.right - measured.left) + kHudTextPadding * 2,
                     y + (measured.bottom - measured.top) + kHudTextPadding * 2 };
    FillRect(m_memDC, &backing, m_blackBrush ? m_blackBrush : (HBRUSH)GetStockObject(BLACK_BRUSH));

    RECT textRect = { backing.left + kHudTextPadding, backing.top + kHudTextPadding,
                      backing.right - kHudTextPadding, backing.bottom - kHudTextPadding };

    int oldMode = SetBkMode(m_memDC, TRANSPARENT);
    COLORREF oldColor = SetTextColor(m_memDC, color);
    DrawTextW(m_memDC, text.c_str(), static_cast<int>(text.size()), &textRect, format);
    SetTextColor(m_memDC, oldColor);
    SetBkMode(m_memDC, oldMode);

    if (oldFont) SelectObject(m_memDC, oldFont);
}
