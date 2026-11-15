// Native replacement of sub_4895e0 (stdcall; stack argument words: exactly 5 (ret 20)): adds four projected 10-entry columns into a 40-float accumulator.

#include <native/common.h>
#include <native/shadow.h>

namespace ug2
{

void sub_4895e0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

// Guest base is page aligned: aligned addresses allow vldr.
Real ldAligned(x86::reg32 address)
{
    float value;
    memcpy(&value, __builtin_assume_aligned(win32::g_guestBase + address, 4), 4);
    return Real(value);
}

void addTo(float* p, Real x)
{
    Real r = x + Real(*p);
    *p = float(r);
}

void subFrom(float* p, Real x)
{
    Real r = Real(*p) - x;
    *p = float(r);
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    (void)app; // the guest leaves eax, edx and esp unchanged in cpu: so does this
    const x86::reg32 m = cpu.ecx;
    const x86::reg32 v = argument(cpu, 0);
    const x86::reg32 args = cpu.esp + 8;
    x86::reg8* const g = win32::g_guestBase;

    // vldr/vstr fault on unaligned addresses: unaligned operands are worked on in copies.
    float accCopy[40], vCopy[4], argCopy[4];
    const bool aligned = ((m | v | args) & 3) == 0;
    float* acc = accCopy;
    const float* pv = vCopy;
    const float* pa = argCopy;
    if (aligned)
    {
        acc = static_cast<float*>(__builtin_assume_aligned(g + m, 4));
        pv = static_cast<const float*>(__builtin_assume_aligned(g + v, 4));
        pa = static_cast<const float*>(__builtin_assume_aligned(g + args, 4));
    }
    else
    {
        memcpy(accCopy, g + m, sizeof(accCopy));
        memcpy(vCopy, g + v, sizeof(vCopy));
        memcpy(argCopy, g + args, sizeof(argCopy));
    }

    const Real a1 = Real(pa[0]);
    const Real a2 = Real(pa[1]);
    const Real a3 = Real(pa[2]);
    const Real a4 = Real(pa[3]);
    const Real vals[4] = {Real(pv[0]), Real(pv[1]), Real(pv[2]), Real(pv[3])};
    const Real K787adc = ldAligned(0x787adc), K784250 = ldAligned(0x784250);
    const Real K91c = ldAligned(0x78d91c), K920 = ldAligned(0x78d920), K924 = ldAligned(0x78d924);
    const Real K928 = ldAligned(0x78d928), K92c = ldAligned(0x78d92c), K930 = ldAligned(0x78d930);
    const Real K934 = ldAligned(0x78d934), K938 = ldAligned(0x78d938), K93c = ldAligned(0x78d93c);
    const Real K940 = ldAligned(0x78d940), K944 = ldAligned(0x78d944);

    Real S = a4 * a4;
    S = S * K787adc;
    S = S - K784250;
    Real D = a2 * a2;
    Real U = a3 * a3;
    D = D - U;
    for (int k = 0; k < 4; k++)
    {
        Real A = vals[k] * a1;
        Real B = K944 * A;
        Real C = A * K940;
        Real E = a3 * C;
        Real F = a4 * C;
        Real G = C * a2;
        Real H = A * K93c;
        Real I = a2 * H;
        Real J = a3 * I;
        Real M = H * a3;
        M = M * a4;
        Real N = I * a4;
        Real O = A * K938;
        O = O * S;
        Real P = A * K934;
        P = P * D;
        Real Q = B * K930;
        Real W = O * K92c;
        Real R = Q - W;
        float* base = acc + k;
        addTo(base, R);
        Real T = P * K928;
        addTo(base + 4, T);
        subFrom(base + 8, T);
        Real X = O * K924;
        addTo(base + 12, X);
        X = J * K920;
        addTo(base + 16, X);
        X = N * K920;
        addTo(base + 20, X);
        X = M * K928;
        addTo(base + 24, X);
        X = G * K91c;
        addTo(base + 28, X);
        X = E * K91c;
        addTo(base + 32, X);
        X = F * K91c;
        addTo(base + 36, X);
    }

    if (!aligned)
        memcpy(g + m, accCopy, sizeof(accCopy));
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back({cpu.ecx, 0xa0, true});
}

const shadow::Spec spec = {"sub_4895e0", shadow::Eax | shadow::Ecx | shadow::Edx | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 4, false, false};

}

void sub_4895e0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_4895e0_guest, declare);
}

}}
