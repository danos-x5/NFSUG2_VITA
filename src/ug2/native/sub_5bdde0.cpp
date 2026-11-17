// Native replacement of sub_5bdde0: per-mesh world, view and projection upload with a three-way cache (cdecl: world, view).

#include <native/common.h>
#include <native/shadow.h>
#include <platform/switches.h>
#include <platform/cached.h>
#include <platform/config.h>
#include <winapi/d3d9_native.h>

namespace ug2
{

void sub_5bdde0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

const float* matrixAt(x86::reg32 address)
{
    return reinterpret_cast<const float*>(rc::host(address));
}

// Bridge calls never write guest memory: reads stay valid.
void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!NFSU_NATIVE("world_upload_native"))
    {
        sub_5bdde0_guest(app, cpu);
        return;
    }
    const x86::reg32 world = argument(cpu, 0);
    const x86::reg32 view = argument(cpu, 1);
    rc::st32(0x872ef4, rc::ld32(0x872ef4) + 1);
    if (world == 0x872e90)
        rc::st32(0x872f00, rc::ld32(0x872f00) + 1);
    const x86::reg32 group = rc::ld32(0x870910);
    const x86::reg32 cachedGroup = rc::ld32(0x87684c);
    const x86::reg32 cachedView = rc::ld32(0x800534);
    if (world == rc::ld32(0x876850) && group == cachedGroup && cachedView == view)
    {
        cpu.eax = group;
        cpu.esp += 4;
        return;
    }
    const x86::reg32 plat = rc::ld32(0x832de0 + view * 0x70);
    const x86::reg32 device = rc::ld32(0x870974);
    const bool viewChanged = view != cachedView;
    if ((viewChanged || group != cachedGroup) && rc::ld8(group + 0x18) == 0)
    {
        win32::d3d9::bridge::setTransform(app, cpu, device, 2, matrixAt(plat));
        win32::d3d9::bridge::setTransform(app, cpu, device, 3, matrixAt(plat + 0x40));
    }
    if (viewChanged)
    {
        rc::st32(0x872efc, rc::ld32(0x872efc) + 1);
        rc::st32(0x800534, view);
    }
    rc::st32(0x872ef8, rc::ld32(0x872ef8) + 1);
    rc::st32(0x87684c, group);
    rc::st32(0x876850, world);
    const float* const worldMatrix = matrixAt(world);
    const x86::reg32 desc = rc::ld32(group + 0x1c);
    const x86::reg32 wvHandle = rc::ld32(desc + 0x934);
    float product[16];
    if (wvHandle != 0)
    {
        win32::d3d9::bridge::matrixMultiply(worldMatrix, matrixAt(plat), product);
        win32::d3d9::bridge::effectSetMatrix(app, cpu, rc::ld32(group + 0x20), wvHandle, product);
    }
    if (rc::ld8(group + 0x18) == 0)
    {
        win32::d3d9::bridge::setTransform(app, cpu, device, 0x100, worldMatrix);
        cpu.eax = group;
        cpu.esp += 4;
        return;
    }
    win32::d3d9::bridge::matrixMultiply(worldMatrix, matrixAt(plat + 0x80), product);
    cpu.eax = win32::d3d9::bridge::effectSetMatrix(app, cpu, rc::ld32(group + 0x20), rc::ld32(desc + 0x95c), product);
    cpu.esp += 4;
}

void declare(x86::CPU&, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{0x872ef4, 16, false});
    spans.push_back(shadow::Span{0x876850, 4, false});
    spans.push_back(shadow::Span{0x87684c, 4, false});
    spans.push_back(shadow::Span{0x800534, 4, false});
}

// ecx and edx are not compared: D3D call leftovers.
const shadow::Spec spec = {"sub_5bdde0", shadow::Eax | shadow::Ebx | shadow::Esp | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, true};

}

void sub_5bdde0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_5bdde0_guest, declare);
}

}}
