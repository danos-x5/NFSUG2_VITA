// The Vita body layer of the Key Binds picture (controls-overhaul's diagram.cpp, body only, no scheme).

#include "vitapicture.h"
#include "diagram_art.h"

#include <algorithm>
#include <cstring>

namespace ug2 { namespace vitapicture {

namespace
{

// Decodes a run-length coded layer (make_layers.py rle) into pixels of bpp bytes.
std::vector<uint8_t> decode(const diagramart::Layer& layer, int bpp)
{
    std::vector<uint8_t> out(size_t(layer.w) * layer.h * bpp, 0);
    size_t at = 0;
    for (int i = 0; i < layer.bytes && at < out.size();)
    {
        const int control = layer.data[i++];
        if (control & 0x80)
            at += size_t(control - 127) * bpp;
        else
        {
            const size_t bytes = size_t(control + 1) * bpp;
            memcpy(&out[at], &layer.data[i], std::min(bytes, out.size() - at));
            at += bytes;
            i += int(bytes);
        }
    }
    return out;
}

void blend(std::vector<uint8_t>& canvas, int x, int y, int w, int h, const std::vector<uint8_t>& pixels, bool rgba)
{
    const int bpp = rgba ? 4 : 2;
    for (int row = 0; row < h; ++row)
    {
        const int cy = y + row;
        if (cy < 0 || cy >= diagramart::Height)
            continue;
        for (int col = 0; col < w; ++col)
        {
            const int cx = x + col;
            if (cx < 0 || cx >= diagramart::Width)
                continue;
            const uint8_t* s = &pixels[(size_t(row) * w + col) * bpp];
            const int sa = rgba ? s[3] : s[1];
            if (!sa)
                continue;
            const int sr = s[0], sg = rgba ? s[1] : s[0], sb = rgba ? s[2] : s[0];
            uint8_t* d = &canvas[(size_t(cy) * diagramart::Width + cx) * 4];
            const int da = d[3];
            const int outA = sa + (da * (255 - sa) + 127) / 255;
            const int weightD = (da * (255 - sa) + 127) / 255;
            for (int c = 0; c < 3; ++c)
            {
                const int sc = c == 0 ? sr : c == 1 ? sg : sb;
                d[c] = uint8_t(outA ? (sc * sa + d[c] * weightD + outA / 2) / outA : 0);
            }
            d[3] = uint8_t(outA);
        }
    }
}

}

int width() { return diagramart::Width; }
int height() { return diagramart::Height; }

std::vector<uint8_t> bodyRgba()
{
    std::vector<uint8_t> canvas(size_t(diagramart::Width) * diagramart::Height * 4, 0);
    const diagramart::Layer& layer = diagramart::Layers[diagramart::BodyLayer];
    blend(canvas, layer.x, layer.y, layer.w, layer.h, decode(layer, layer.rgba ? 4 : 2), layer.rgba);
    return canvas;
}

std::vector<uint8_t> encodeDxt3(const uint8_t* rgba, int w, int h)
{
    std::vector<uint8_t> out;
    out.reserve(size_t(w) * h);
    for (int by = 0; by < h; by += 4)
    {
        for (int bx = 0; bx < w; bx += 4)
        {
            const uint8_t* px[16];
            for (int i = 0; i < 16; ++i)
                px[i] = rgba + (size_t(by + i / 4) * w + bx + i % 4) * 4;
            for (int i = 0; i < 16; i += 2)
            {
                const int a0 = (px[i][3] * 15 + 127) / 255, a1 = (px[i + 1][3] * 15 + 127) / 255;
                out.push_back(uint8_t(a0 | (a1 << 4)));
            }
            int lo = -1, hi = -1, lowLum = 1 << 30, highLum = -1;
            for (int i = 0; i < 16; ++i)
            {
                if (!px[i][3])
                    continue;
                const int lum = px[i][0] * 3 + px[i][1] * 6 + px[i][2];
                if (lum < lowLum) { lowLum = lum; lo = i; }
                if (lum > highLum) { highLum = lum; hi = i; }
            }
            auto pack = [](const uint8_t* p) { return uint16_t(((p[0] >> 3) << 11) | ((p[1] >> 2) << 5) | (p[2] >> 3)); };
            uint16_t c0 = hi >= 0 ? pack(px[hi]) : 0, c1 = lo >= 0 ? pack(px[lo]) : 0;
            if (c0 < c1)
                std::swap(c0, c1);
            auto expand = [](uint16_t v, int c) {
                const int bits = c == 1 ? 6 : 5, shift = c == 0 ? 11 : c == 1 ? 5 : 0;
                const int x = (v >> shift) & ((1 << bits) - 1);
                return (x << (8 - bits)) | (x >> (2 * bits - 8));
            };
            int palette[4][3];
            for (int c = 0; c < 3; ++c)
            {
                palette[0][c] = expand(c0, c);
                palette[1][c] = expand(c1, c);
                palette[2][c] = (2 * palette[0][c] + palette[1][c]) / 3;
                palette[3][c] = (palette[0][c] + 2 * palette[1][c]) / 3;
            }
            uint32_t indices = 0;
            if (c0 != c1)
            {
                for (int i = 0; i < 16; ++i)
                {
                    int best = 0, bestDistance = 1 << 30;
                    for (int k = 0; k < 4; ++k)
                    {
                        int d = 0;
                        for (int c = 0; c < 3; ++c)
                            d += (px[i][c] - palette[k][c]) * (px[i][c] - palette[k][c]);
                        if (d < bestDistance) { bestDistance = d; best = k; }
                    }
                    indices |= uint32_t(best) << (2 * i);
                }
            }
            out.push_back(uint8_t(c0)); out.push_back(uint8_t(c0 >> 8));
            out.push_back(uint8_t(c1)); out.push_back(uint8_t(c1 >> 8));
            for (int i = 0; i < 4; ++i)
                out.push_back(uint8_t(indices >> (8 * i)));
        }
    }
    return out;
}

}}
