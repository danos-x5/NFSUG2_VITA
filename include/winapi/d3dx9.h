#ifndef WINAPI_D3DX9_H_
#define WINAPI_D3DX9_H_

// D3DX 9.0c natives for SPEED2.EXE (games/ug2.py NATIVE_OVERRIDES), stdcall with the guest ABI: CreateEffectPool
// 0x640AAC, CreateEffectFromResourceA 0x640C02, MatrixMultiply 0x63CEDB, MatrixTranspose 0x63D12C.

#include <x86.h>

namespace win32
{

class WinApplication;

}

namespace win32 { namespace d3dx9
{

void CreateEffectPool(WinApplication* app, x86::CPU& cpu);
void CreateEffectFromResourceA(WinApplication* app, x86::CPU& cpu);
void MatrixMultiply(WinApplication* app, x86::CPU& cpu);
void MatrixTranspose(WinApplication* app, x86::CPU& cpu);
void Vec3Transform(WinApplication* app, x86::CPU& cpu);
void Vec4Transform(WinApplication* app, x86::CPU& cpu);

// The arithmetic of Vec3Transform and Vec4Transform: r = v * m, m as 16 floats row by row (v[3] = 1 for Vec3).
void transform(const float v[4], const float m[16], float r[4]);

}}

#endif
