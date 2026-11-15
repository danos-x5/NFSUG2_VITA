#ifndef gdi32_H_
#define gdi32_H_

#include <x86.h>
#include <winapi/types.h>

namespace win32 { class WinApplication; }

// GDI: memory device contexts with bitmaps that keep their pixels, and fonts that are only handles. Text is not
// rasterised (logged once); window DCs draw nothing.
namespace win32 { namespace gdi32
{

typedef DWORD COLORREF;

int GetDeviceCaps(WinApplication* app, x86::CPU& cpu, HDC hdc, int index);
HGDIOBJ GetStockObject(WinApplication* app, x86::CPU& cpu, int i);
BOOL DeleteObject(WinApplication* app, x86::CPU& cpu, HGDIOBJ ho);
struct TEXTMETRICA;
BOOL GetTextMetricsA(WinApplication* app, x86::CPU& cpu, HDC hdc, TEXTMETRICA* lptm);
HGDIOBJ SelectObject(WinApplication* app, x86::CPU& cpu, HDC hdc, HGDIOBJ h);
BOOL TextOutA(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y, LPCSTR lpString, int c);
HDC CreateCompatibleDC(WinApplication* app, x86::CPU& cpu, HDC hdc);
BOOL DeleteDC(WinApplication* app, x86::CPU& cpu, HDC hdc);
HGDIOBJ CreateBitmap(WinApplication* app, x86::CPU& cpu, int nWidth, int nHeight, UINT nPlanes, UINT nBitCount,
                     x86::reg32 lpBits);
HGDIOBJ CreateFontA(WinApplication* app, x86::CPU& cpu, int cHeight, int cWidth, int cEscapement, int cOrientation,
                    int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet,
                    DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily,
                    LPCSTR pszFaceName);
COLORREF SetTextColor(WinApplication* app, x86::CPU& cpu, HDC hdc, COLORREF color);
COLORREF SetBkColor(WinApplication* app, x86::CPU& cpu, HDC hdc, COLORREF color);
int SetBkMode(WinApplication* app, x86::CPU& cpu, HDC hdc, int mode);
BOOL ExtTextOutA(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y, UINT options, const RECT* lprect,
                 LPCSTR lpString, UINT c, x86::reg32 lpDx);
COLORREF GetPixel(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y);
BOOL BitBlt(WinApplication* app, x86::CPU& cpu, HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1,
            DWORD rop);

}}

#endif
