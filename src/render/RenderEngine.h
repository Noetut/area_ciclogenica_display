#ifndef RENDER_ENGINE_H
#define RENDER_ENGINE_H

#include <windows.h>
#include <gdiplus.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "model/ProjectionArea.h"

class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    bool Initialize(HWND hwnd, int width, int height);
    void Cleanup();
    void Resize(int width, int height);

    void BeginFrame();
    void EndFrame();

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    // --- Show mode --------------------------------------------------------
    void RenderBlack();
    void DrawBackgroundVideo(const BYTE* pixels, int videoWidth, int videoHeight);
    void RenderAreas(const std::vector<ProjectionArea>& areas,
                     const BYTE* bgVideoPixels = nullptr, int bgVideoWidth = 0, int bgVideoHeight = 0);

    // --- Image & Text rendering ------------------------------------------
    void DrawQuadImage(const Quad& quad, Gdiplus::Bitmap* bitmap);
    Gdiplus::Bitmap* GetOrLoadImage(const std::string& path);
    void DrawQuadText(const Quad& quad, const std::string& text,
                      const std::string& fontFace = "Arial", int fontSize = 32,
                      COLORREF color = RGB(255, 255, 255));

    // --- Calibration primitives -------------------------------------------
    // Solid fill. Axis-aligned quads take a FillRect fast path so that
    // uncalibrated areas keep exactly the pixel coverage they had before.
    void FillQuad(const Quad& quad, COLORREF color);
    // 50% checkerboard fill: GDI has no alpha channel, so this is how the
    // selected area is highlighted without blinding the room.
    void FillQuadHalftone(const Quad& quad, COLORREF color);
    void DrawQuadOutline(const Quad& quad, COLORREF color, int thickness);
    void DrawHandle(const Point2i& point, int halfSize, COLORREF fill, COLORREF border);
    void DrawCross(const Point2i& point, int armLength, COLORREF color);
    // Text with a semi-transparent backing box and semi-transparent text,
    // so calibrated areas underneath remain visible while keeping text readable.
    void DrawHudText(int x, int y, const std::wstring& text, COLORREF color,
                     BYTE bgAlpha = 120, BYTE textAlpha = 180);

    HDC GetBackBufferDC() const { return m_memDC; }

private:
    struct CachedPen {
        COLORREF color;
        int      thickness;
        HPEN     pen;
    };

    // Pens are cached because the calibration overlay redraws every outline on
    // every one of the 60 frames per second.
    HPEN GetPen(COLORREF color, int thickness);
    void FillQuadWithBrush(const Quad& quad, HBRUSH brush);
    void EnsureScratchBuffer(int minWidth, int minHeight);

    HWND    m_hwnd;
    HDC     m_hdc;
    HDC     m_memDC;
    HBITMAP m_hBitmap;
    HBITMAP m_hOldBitmap;
    HBRUSH  m_blackBrush;
    HBRUSH  m_whiteBrush;
    HBRUSH  m_halftoneBrush;
    HFONT   m_hudFont;
    std::vector<CachedPen> m_pens;
    int     m_width;
    int     m_height;

    // Reusable scratch 32-bit DIB buffer for alpha-blended HUD text rendering
    HDC     m_scratchDC;
    HBITMAP m_scratchBitmap;
    HBITMAP m_scratchOldBitmap;
    void*   m_scratchBits;
    int     m_scratchWidth;
    int     m_scratchHeight;

    void LoadCustomFonts();

    ULONG_PTR m_gdiplusToken;
    Gdiplus::PrivateFontCollection* m_fontCollection;
    std::unordered_map<std::string, Gdiplus::Bitmap*> m_imageCache;
};

#endif // RENDER_ENGINE_H
