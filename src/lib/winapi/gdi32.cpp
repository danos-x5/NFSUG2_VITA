#include <winapi/gdi32.h>
#include <lib/winapp.h>
#include <x86.h>
#include <platform/log.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace win32 { namespace gdi32
{

// Logs an error once per call site.
#define GDI_LOG_ONCE(...)                                                           \
    do { static bool logged_ = false; if (!logged_) { logged_ = true; pf::logError(__VA_ARGS__); } } while (0)

// The desktop is the 16-bit Vita display.
int GetDeviceCaps(WinApplication* app, x86::CPU& cpu, HDC hdc, int index)
{
    RC_USE(app);
    RC_USE(cpu);
    RC_USE(hdc);
    enum { HORZRES = 8, VERTRES = 10, BITSPIXEL = 12, PLANES = 14 };
    switch (index)
    {
    case HORZRES:
        return 960;
    case VERTRES:
        return 544;
    case BITSPIXEL:
        return 16;
    case PLANES:
        return 1;
    default:
        pf::logError("GetDeviceCaps: index %d not implemented", index);
        return 0;
    }
}

// Window DCs have no target on the Vita; only the game's debug log window draws on them.
struct TEXTMETRICA
{
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
    BYTE tmFirstChar;
    BYTE tmLastChar;
    BYTE tmDefaultChar;
    BYTE tmBreakChar;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
};

// The metrics of an 8x16 fixed font.
BOOL GetTextMetricsA(WinApplication* app, x86::CPU& cpu, HDC hdc, TEXTMETRICA* lptm)
{
    RC_USE(app);
    RC_USE(cpu);
    RC_USE(hdc);
    if (!lptm)
        return 0;
    memset(lptm, 0, sizeof(*lptm));
    lptm->tmHeight = 16;
    lptm->tmAscent = 12;
    lptm->tmDescent = 4;
    lptm->tmAveCharWidth = 8;
    lptm->tmMaxCharWidth = 8;
    lptm->tmWeight = 400;
    lptm->tmLastChar = 0xff;
    lptm->tmDefaultChar = '?';
    lptm->tmBreakChar = ' ';
    return 1;
}


BOOL TextOutA(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y, LPCSTR lpString, int c)
{
    RC_USE(app);
    RC_USE(cpu);
    RC_USE(hdc);
    RC_USE(x);
    RC_USE(y);
    RC_USE(lpString);
    RC_USE(c);
    return 1;
}

// Stock objects WHITE_BRUSH (0) .. DC_PEN (19) are pseudo handles; nothing draws with them.
static const HGDIOBJ StockObjectBase = 0xfffc0000;
static const HGDIOBJ SystemFont = StockObjectBase + 13;
// The 1x1 bitmap a new memory DC holds, a pseudo handle too.
static const HGDIOBJ DefaultBitmap = StockObjectBase + 20;
static const HGDIOBJ HgdiError = 0xffffffff;
static const COLORREF ClrInvalid = 0xffffffff;
static const COLORREF White = 0xffffff;

static bool isStock(HGDIOBJ h)
{
    return h >= StockObjectBase && h <= DefaultBitmap;
}

namespace
{

// A memory DC, a bitmap (COLORREF pixels, rows top down) or a font.
class GdiObject : public GenericResource
{
public:
    enum Kind
    {
        Dc,
        Bitmap,
        Font
    };

    explicit GdiObject(Kind kind) : kind(kind) {}

    const Kind              kind;
    HGDIOBJ                 bitmap = DefaultBitmap;
    HGDIOBJ                 font = SystemFont;
    COLORREF                textColor = 0;
    COLORREF                bkColor = White;
    int                     bkMode = 2;     // OPAQUE
    int                     width = 0;
    int                     height = 0;
    std::vector<COLORREF>   pixels;
};

GdiObject* find(WinApplication* app, x86::reg32 handle, GdiObject::Kind kind)
{
    GdiObject* object = isStock(handle) ? nullptr : dynamic_cast<GdiObject*>(app->getResource(handle));
    return object && object->kind == kind ? object : nullptr;
}

// The bitmap selected into a memory DC; null for another DC or the default bitmap.
GdiObject* target(WinApplication* app, HDC hdc)
{
    GdiObject* dc = find(app, hdc, GdiObject::Dc);
    return dc ? find(app, dc->bitmap, GdiObject::Bitmap) : nullptr;
}

// Fills [left, right) x [top, bottom), clipped to the bitmap.
void fill(GdiObject* bitmap, int64_t left, int64_t top, int64_t right, int64_t bottom, COLORREF color)
{
    const int x0 = int(std::clamp<int64_t>(left, 0, bitmap->width));
    const int x1 = int(std::clamp<int64_t>(right, 0, bitmap->width));
    const int y0 = int(std::clamp<int64_t>(top, 0, bitmap->height));
    const int y1 = int(std::clamp<int64_t>(bottom, 0, bitmap->height));
    if (x0 >= x1)
        return;
    for (int y = y0; y < y1; ++y)
    {
        COLORREF* row = bitmap->pixels.data() + size_t(y) * size_t(bitmap->width);
        std::fill(row + x0, row + x1, color);
    }
}

}

HGDIOBJ GetStockObject(WinApplication* app, x86::CPU& cpu, int i)
{
    RC_USE(app);
    RC_USE(cpu);
    return i >= 0 && i <= 19 ? StockObjectBase + HGDIOBJ(i) : 0;
}

// Deleting a stock object succeeds and does nothing, as on Windows.
BOOL DeleteObject(WinApplication* app, x86::CPU& cpu, HGDIOBJ ho)
{
    RC_USE(cpu);
    if (isStock(ho))
        return 1;
    if (!find(app, ho, GdiObject::Bitmap) && !find(app, ho, GdiObject::Font))
        return 0;
    return app->freeResource(ho) ? 1 : 0;
}

// Window DCs keep nothing selected: HGDI_ERROR.
HGDIOBJ SelectObject(WinApplication* app, x86::CPU& cpu, HDC hdc, HGDIOBJ h)
{
    RC_USE(cpu);
    GdiObject* dc = find(app, hdc, GdiObject::Dc);
    if (!dc)
        return HgdiError;
    HGDIOBJ previous;
    if (h == DefaultBitmap || find(app, h, GdiObject::Bitmap))
    {
        previous = dc->bitmap;
        dc->bitmap = h;
    }
    else if ((h >= StockObjectBase + 10 && h <= StockObjectBase + 17) || find(app, h, GdiObject::Font))
    {
        previous = dc->font;
        dc->font = h;
    }
    else
    {
        // Brushes and pens are stock objects that draw nothing.
        previous = isStock(h) ? h : 0;
    }
    return previous;
}

HDC CreateCompatibleDC(WinApplication* app, x86::CPU& cpu, HDC hdc)
{
    RC_USE(cpu);
    RC_USE(hdc);
    return app->allocateResource(new GdiObject(GdiObject::Dc));
}

BOOL DeleteDC(WinApplication* app, x86::CPU& cpu, HDC hdc)
{
    RC_USE(cpu);
    return find(app, hdc, GdiObject::Dc) && app->freeResource(hdc) ? 1 : 0;
}

// Monochrome bits (word-aligned rows, 1 is white) are read; other formats start black.
HGDIOBJ CreateBitmap(WinApplication* app, x86::CPU& cpu, int nWidth, int nHeight, UINT nPlanes, UINT nBitCount,
                     x86::reg32 lpBits)
{
    RC_USE(cpu);
    if (nWidth <= 0 || nHeight <= 0 || nWidth > 16384 || nHeight > 16384)
        return 0;
    GdiObject* bitmap = new GdiObject(GdiObject::Bitmap);
    bitmap->width = nWidth;
    bitmap->height = nHeight;
    bitmap->pixels.assign(size_t(nWidth) * size_t(nHeight), 0);
    if (lpBits && nPlanes == 1 && nBitCount == 1)
    {
        const x86::reg32 stride = x86::reg32((nWidth + 15) / 16 * 2);
        for (int y = 0; y < nHeight; ++y)
        {
            for (int x = 0; x < nWidth; ++x)
            {
                const x86::reg8 bits = app->getMemory<x86::reg8>(lpBits + x86::reg32(y) * stride + x86::reg32(x / 8));
                if (bits & (0x80 >> (x % 8)))
                    bitmap->pixels[size_t(y) * size_t(nWidth) + size_t(x)] = White;
            }
        }
    }
    else if (lpBits)
    {
        GDI_LOG_ONCE("CreateBitmap: %u planes of %u bits not read, the bitmap starts black", nPlanes, nBitCount);
    }
    return app->allocateResource(bitmap);
}

HGDIOBJ CreateFontA(WinApplication* app, x86::CPU& cpu, int cHeight, int cWidth, int cEscapement, int cOrientation,
                    int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet,
                    DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily,
                    LPCSTR pszFaceName)
{
    RC_USE(cpu);
    RC_USE(cWidth);
    RC_USE(cEscapement);
    RC_USE(cOrientation);
    RC_USE(cWeight);
    RC_USE(bItalic);
    RC_USE(bUnderline);
    RC_USE(bStrikeOut);
    RC_USE(iCharSet);
    RC_USE(iOutPrecision);
    RC_USE(iClipPrecision);
    RC_USE(iQuality);
    pf::logDebug("CreateFontA: height %d, pitch and family 0x%x, face \"%s\"", cHeight, unsigned(iPitchAndFamily),
                 pszFaceName ? pszFaceName : "");
    return app->allocateResource(new GdiObject(GdiObject::Font));
}

COLORREF SetTextColor(WinApplication* app, x86::CPU& cpu, HDC hdc, COLORREF color)
{
    RC_USE(cpu);
    GdiObject* dc = find(app, hdc, GdiObject::Dc);
    if (!dc)
        return ClrInvalid;
    const COLORREF previous = dc->textColor;
    dc->textColor = color;
    return previous;
}

COLORREF SetBkColor(WinApplication* app, x86::CPU& cpu, HDC hdc, COLORREF color)
{
    RC_USE(cpu);
    GdiObject* dc = find(app, hdc, GdiObject::Dc);
    if (!dc)
        return ClrInvalid;
    const COLORREF previous = dc->bkColor;
    dc->bkColor = color;
    return previous;
}

int SetBkMode(WinApplication* app, x86::CPU& cpu, HDC hdc, int mode)
{
    RC_USE(cpu);
    GdiObject* dc = find(app, hdc, GdiObject::Dc);
    if (!dc || (mode != 1 && mode != 2))
        return 0;
    const int previous = dc->bkMode;
    dc->bkMode = mode;
    return previous;
}

// ETO_OPAQUE fills the rectangle with the background colour; the glyphs are not drawn.
BOOL ExtTextOutA(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y, UINT options, const RECT* lprect,
                 LPCSTR lpString, UINT c, x86::reg32 lpDx)
{
    RC_USE(cpu);
    RC_USE(x);
    RC_USE(y);
    RC_USE(lpDx);
    static const UINT ETO_OPAQUE = 2;
    GdiObject* dc = find(app, hdc, GdiObject::Dc);
    GdiObject* bitmap = target(app, hdc);
    if (bitmap && lprect && (options & ETO_OPAQUE))
        fill(bitmap, lprect->left, lprect->top, lprect->right, lprect->bottom, dc->bkColor);
    if (c && lpString)
        GDI_LOG_ONCE("ExtTextOutA: text is not rasterised by the port (first: \"%.*s\")", int(std::min(c, 80u)),
                         lpString);
    return 1;
}

COLORREF GetPixel(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y)
{
    RC_USE(cpu);
    GdiObject* bitmap = target(app, hdc);
    if (!bitmap || x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height)
        return ClrInvalid;
    return bitmap->pixels[size_t(y) * size_t(bitmap->width) + size_t(x)];
}

// BLACKNESS, WHITENESS and SRCCOPY between memory DCs; a window DC as the target draws nothing.
BOOL BitBlt(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1,
            DWORD rop)
{
    RC_USE(cpu);
    static const DWORD BLACKNESS = 0x00000042, WHITENESS = 0x00ff0062, SRCCOPY = 0x00cc0020;
    if (!find(app, hdc, GdiObject::Dc))
        return 1;
    GdiObject* bitmap = target(app, hdc);
    if (!bitmap)
        return 1;
    if (rop == BLACKNESS || rop == WHITENESS)
    {
        fill(bitmap, x, y, int64_t(x) + cx, int64_t(y) + cy, rop == WHITENESS ? White : 0);
        return 1;
    }
    GdiObject* source = rop == SRCCOPY ? target(app, hdcSrc) : nullptr;
    if (!source)
    {
        GDI_LOG_ONCE("BitBlt: raster operation 0x%08x or its source DC is not supported by the port", unsigned(rop));
        return 0;
    }
    const int64_t c0 = std::max<int64_t>({0, -int64_t(x), -int64_t(x1)});
    const int64_t c1 = std::min<int64_t>({cx, int64_t(bitmap->width) - x, int64_t(source->width) - x1});
    const int64_t r0 = std::max<int64_t>({0, -int64_t(y), -int64_t(y1)});
    const int64_t r1 = std::min<int64_t>({cy, int64_t(bitmap->height) - y, int64_t(source->height) - y1});
    // A blit within one bitmap reads a copy, so overlapping rectangles copy correctly.
    const std::vector<COLORREF> overlap = source == bitmap ? source->pixels : std::vector<COLORREF>();
    const COLORREF* from = source == bitmap ? overlap.data() : source->pixels.data();
    for (int64_t row = r0; row < r1; ++row)
    {
        for (int64_t column = c0; column < c1; ++column)
            bitmap->pixels[size_t(y + row) * size_t(bitmap->width) + size_t(x + column)] =
                from[size_t(y1 + row) * size_t(source->width) + size_t(x1 + column)];
    }
    return 1;
}

}}
