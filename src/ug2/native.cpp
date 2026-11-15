// Native replacements of SPEED2.EXE functions (tools/recompiler/games/ug2.py NATIVE_OVERRIDES). Each returns as
// the guest does: on entry cpu.esp points at the return address.

#include <lib/guest.h>
#include <lib/winapp.h>
#include <platform/gxm.h>
#include <platform/log.h>
#include <winapi/d3dx9.h>
#include <native/common.h>

#include <cstring>
#include <fpu.h>
#include <native.h>
#include <recomp.h>

namespace ug2 { namespace native
{

namespace
{

// memcpy/memmove(dst, src, n): the CRT copies overlaps like memmove.
void moveMemory(x86::CPU& cpu)
{
    const x86::reg32 dst = argument(cpu, 0), src = argument(cpu, 1), n = argument(cpu, 2);
    if (n)
        memmove(::native::host(dst), ::native::host(src), n);
    cpu.eax = dst;
    cpu.esp += 4;
}

}

// _ftol2: st0 to edx:eax, truncated.
void sub_75bed4(win32::WinApplication*, x86::CPU& cpu)
{
    const double value = double(cpu.fpu.pop());
    cpu.edx_eax = x86::reg64(x86::truncInteger64(value));
    cpu.esp += 4;
}

// _chkstk: allocates eax bytes; eax = the return address.
void sub_75c4d0(win32::WinApplication*, x86::CPU& cpu)
{
    const x86::reg32 slot = cpu.esp;
    const x86::reg32 size = cpu.eax;
    cpu.eax = rc::ld32(slot);
    cpu.esp = slot + 4 - size;
}

// Cyrix probe: the translated div keeps flags; answer "not Cyrix".
void sub_6f5fc5(win32::WinApplication*, x86::CPU& cpu)
{
    cpu.eax = 0;
    cpu.esp += 4;
}

// CRT memcpy: its backward-copy jump table traps when translated.
void sub_75cfd0(win32::WinApplication*, x86::CPU& cpu)
{
    moveMemory(cpu);
}

// CRT memmove: the same code as memcpy.
void sub_760df0(win32::WinApplication*, x86::CPU& cpu)
{
    moveMemory(cpu);
}

// D3DX natives: effects and matrix helpers (d3dx9.cpp).
void sub_640aac(win32::WinApplication* app, x86::CPU& cpu)
{
    win32::d3dx9::CreateEffectPool(app, cpu);
}

void sub_640c02(win32::WinApplication* app, x86::CPU& cpu)
{
    win32::d3dx9::CreateEffectFromResourceA(app, cpu);
}

void sub_63cedb(win32::WinApplication* app, x86::CPU& cpu)
{
    win32::d3dx9::MatrixMultiply(app, cpu);
}

void sub_63d12c(win32::WinApplication* app, x86::CPU& cpu)
{
    win32::d3dx9::MatrixTranspose(app, cpu);
}

}}
