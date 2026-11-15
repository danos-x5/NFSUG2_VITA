// Native sub_43d610: grows a bounding box (min, max pointers) by a sphere centre and radius (cdecl: min, max, centre, radius).

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_43d610_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

signed char s_on;

template< bool A >
inline void grow(x86::CPU& cpu, x86::reg32 low, x86::reg32 high, x86::reg32 centre)
{
    const Real radius = fast::lf< A >(cpu.esp + 16);
    const Real cx = fast::lf< A >(centre), cy = fast::lf< A >(centre + 4), cz = fast::lf< A >(centre + 8);
    const float minX = float(cx - radius);
    const float minY = float(cy - radius);
    const float minZ = float(cz - radius);
    const float maxX = float(radius + cx);
    const float maxY = float(radius + cy);
    const float maxZ = float(radius + cz);
#ifdef NFSU_NATIVE_SHADOW
    // The guest keeps two temporaries in its argument slots; the shadow compare reruns on those slots.
    fast::sf< A >(cpu.esp + 12, minY);
    fast::sf< A >(cpu.esp + 16, maxZ);
#endif
    if (Real(minX) < fast::lf< A >(low))
        fast::sf< A >(low, minX);
    if (Real(minY) < fast::lf< A >(low + 4))
        fast::sf< A >(low + 4, minY);
    if (Real(minZ) < fast::lf< A >(low + 8))
    {
        fast::sf< A >(low + 8, minZ);
        cpu.edx = bits(minZ);
    }
    if (Real(maxX) > fast::lf< A >(high))
        fast::sf< A >(high, maxX);
    if (Real(maxY) > fast::lf< A >(high + 4))
    {
        fast::sf< A >(high + 4, maxY);
        cpu.edx = bits(maxY);
    }
    if (Real(maxZ) > fast::lf< A >(high + 8))
        fast::sf< A >(high + 8, maxZ);
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!fast::enabled(s_on, "vec_helpers_native")) [[unlikely]]
    {
        sub_43d610_guest(app, cpu);
        return;
    }
    const x86::reg32 low = argument(cpu, 0);
    const x86::reg32 high = argument(cpu, 1);
    const x86::reg32 centre = argument(cpu, 2);
    if (fast::aligned(low | high | centre | cpu.esp)) [[likely]]
        grow< true >(cpu, low, high, centre);
    else
        grow< false >(cpu, low, high, centre);
    cpu.ecx = high;
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back(shadow::Span{argument(cpu, 0), 12, false});
    spans.push_back(shadow::Span{argument(cpu, 1), 12, false});
    spans.push_back(shadow::Span{cpu.esp + 12, 8, false});
}

const shadow::Spec spec = {"sub_43d610", shadow::Ecx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 0, false, false};

}

void sub_43d610(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_43d610_guest, declare);
}

}}
