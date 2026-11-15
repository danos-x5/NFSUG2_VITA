// Native sub_60cee0: one monotone-chain half of the car shadow hull (cdecl; point pointers, count, comparator).
// The CRT qsort 0x75cc70 runs inline, compare for compare and swap for swap.

#include <native/common.h>
#include <native/fast.h>
#include <native/shadow.h>

namespace ug2
{

void sub_60cee0_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

signed char s_on;

// Elements in guest memory: positions are guest addresses of point pointers.
struct InPlace
{
    typedef x86::reg32 Pos;
    typedef x86::sreg32 Diff;
    static const Pos Step = 4;
    Real zero;

    // Comparator 0x60ce60.
    int order(Pos a, Pos b) const
    {
        const x86::reg32 pa = rc::ld32(a);
        const x86::reg32 pb = rc::ld32(b);
        const Real dx = ld(pa) - ld(pb);
        if (dx > zero)
            return 1;
        if (dx < zero)
            return -1;
        const Real dy = ld(pb + 4) - ld(pa + 4);
        if (dy > zero)
            return 1;
        if (dy < zero)
            return -1;
        return 0;
    }

    void exchange(Pos a, Pos b) const
    {
        const x86::reg32 t = rc::ld32(a);
        rc::st32(a, rc::ld32(b));
        rc::st32(b, t);
    }
};

struct Key
{
    float x, y;
    x86::reg32 point, unused;
};

// Elements copied out with their coordinates: positions are host addresses of keys.
struct Keyed
{
    typedef uintptr_t Pos;
    typedef intptr_t Diff;
    static const Pos Step = sizeof(Key);
    Real zero;

    int order(Pos a, Pos b) const
    {
        const Key* ka = reinterpret_cast< const Key* >(a);
        const Key* kb = reinterpret_cast< const Key* >(b);
        const Real dx = Real(ka->x) - Real(kb->x);
        if (dx > zero)
            return 1;
        if (dx < zero)
            return -1;
        const Real dy = Real(kb->y) - Real(ka->y);
        if (dy > zero)
            return 1;
        if (dy < zero)
            return -1;
        return 0;
    }

    void exchange(Pos a, Pos b) const
    {
        Key* ka = reinterpret_cast< Key* >(a);
        Key* kb = reinterpret_cast< Key* >(b);
        const Key t = *ka;
        *ka = *kb;
        *kb = t;
    }
};

// CRT short sort (selection sort) of 0x75cc00.
template< typename Ops, bool Reversed >
inline void sortShort(const Ops& ops, typename Ops::Pos lo, typename Ops::Pos hi)
{
    while (hi > lo)
    {
        typename Ops::Pos largest = lo;
        for (typename Ops::Pos p = lo + Ops::Step; p <= hi; p += Ops::Step)
        {
            if ((Reversed ? ops.order(largest, p) : ops.order(p, largest)) > 0)
                largest = p;
        }
        if (largest != hi)
            ops.exchange(largest, hi);
        hi -= Ops::Step;
    }
}

// CRT qsort of 0x75cc70, compare for compare and swap for swap.
template< typename Ops, bool Reversed >
void sort(const Ops& ops, typename Ops::Pos base, x86::reg32 count)
{
    typedef typename Ops::Pos Pos;
    typedef typename Ops::Diff Diff;
    const Pos Step = Ops::Step;
    const auto compare = [&ops](Pos a, Pos b) { return Reversed ? ops.order(b, a) : ops.order(a, b); };
    if (count < 2)
        return;
    Pos loStack[30];
    Pos hiStack[30];
    int depth = 0;
    Pos lo = base;
    Pos hi = base + (count - 1) * Step;
    for (;;)
    {
        const Pos size = (hi - lo) / Step + 1;
        if (size <= 8)
        {
            sortShort< Ops, Reversed >(ops, lo, hi);
        }
        else
        {
            Pos mid = lo + (size >> 1) * Step;
            if (compare(lo, mid) > 0)
                ops.exchange(lo, mid);
            if (compare(lo, hi) > 0)
                ops.exchange(lo, hi);
            if (compare(mid, hi) > 0)
                ops.exchange(mid, hi);
            Pos low = lo;
            Pos high = hi;
            for (;;)
            {
                bool passed = true;
                if (mid > low)
                {
                    for (;;)
                    {
                        low += Step;
                        if (low >= mid)
                            break;
                        if (compare(low, mid) > 0)
                        {
                            passed = false;
                            break;
                        }
                    }
                }
                if (passed)
                {
                    do
                    {
                        low += Step;
                    } while (low <= hi && compare(low, mid) <= 0);
                }
                do
                {
                    high -= Step;
                } while (high > mid && compare(high, mid) > 0);
                if (low > high)
                    break;
                if (low != high)
                    ops.exchange(low, high);
                if (mid == high)
                    mid = low;
            }
            high += Step;
            bool found = false;
            if (mid < high)
            {
                for (;;)
                {
                    high -= Step;
                    if (high <= mid)
                        break;
                    if (compare(high, mid) != 0)
                    {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
            {
                do
                {
                    high -= Step;
                } while (high > lo && compare(high, mid) == 0);
            }
            if (Diff(high - lo) >= Diff(hi - low))
            {
                if (lo < high)
                {
                    loStack[depth] = lo;
                    hiStack[depth] = high;
                    ++depth;
                }
                if (low < hi)
                {
                    lo = low;
                    continue;
                }
            }
            else
            {
                if (low < hi)
                {
                    loStack[depth] = low;
                    hiStack[depth] = hi;
                    ++depth;
                }
                if (lo < high)
                {
                    hi = high;
                    continue;
                }
            }
        }
        if (--depth < 0)
            return;
        lo = loStack[depth];
        hi = hiStack[depth];
    }
}

// Sort and chain on the guest's pointer array.
x86::sreg32 hullInPlace(x86::reg32 points, x86::sreg32 count, bool reversed)
{
    const Real zero = ld(0x7d7d3c);
    const InPlace ops = {zero};
    if (reversed)
        sort< InPlace, true >(ops, points, x86::reg32(count));
    else
        sort< InPlace, false >(ops, points, x86::reg32(count));

    x86::sreg32 top = 1;
    for (x86::sreg32 i = 2; i < count; ++i)
    {
        x86::sreg32 k = top;
        if (k >= 1)
        {
            const x86::reg32 p = rc::ld32(points + i * 4);
            const Real px = ld(p);
            const Real py = ld(p + 4);
            do
            {
                const x86::reg32 a = rc::ld32(points + k * 4);
                const x86::reg32 d = rc::ld32(points + k * 4 - 4);
                const Real ax = ld(a);
                const Real ay = ld(a + 4);
                Real left = ld(d + 4) - ay;
                left *= px - ax;
                Real right = py - ay;
                right *= ld(d) - ax;
                left -= right;
                if (!(left <= zero))
                    break;
                --k;
            } while (k >= 1);
        }
        const x86::reg32 kept = rc::ld32(points + k * 4 + 4);
        top = k + 1;
        rc::st32(points + top * 4, rc::ld32(points + i * 4));
        rc::st32(points + i * 4, kept);
    }
    return top;
}

const x86::sreg32 MostKeys = 256;

// Sort and chain on a copy that carries each point's coordinates; false when the array overlaps what it reads.
bool hullKeyed(x86::reg32 points, x86::sreg32 count, bool reversed, x86::sreg32& result)
{
    const x86::reg32 bytes = x86::reg32(count) * 4;
    if (0x7d7d3c - (points - 3) < bytes + 3)
        return false;
    Key keys[MostKeys];
    for (x86::sreg32 i = 0; i < count; ++i)
    {
        const x86::reg32 p = rc::ld32(points + i * 4);
        if (p - (points - 7) < bytes + 7)
            return false;
        memcpy(&keys[i].x, rc::host(p), 8);
        keys[i].point = p;
    }
    const Real zero = ld(0x7d7d3c);
    const Keyed ops = {zero};
    if (reversed)
        sort< Keyed, true >(ops, uintptr_t(keys), x86::reg32(count));
    else
        sort< Keyed, false >(ops, uintptr_t(keys), x86::reg32(count));

    x86::sreg32 top = 1;
    for (x86::sreg32 i = 2; i < count; ++i)
    {
        x86::sreg32 k = top;
        const Real px = Real(keys[i].x);
        const Real py = Real(keys[i].y);
        while (k >= 1)
        {
            const Real ax = Real(keys[k].x);
            const Real ay = Real(keys[k].y);
            Real left = Real(keys[k - 1].y) - ay;
            left *= px - ax;
            Real right = py - ay;
            right *= Real(keys[k - 1].x) - ax;
            left -= right;
            if (!(left <= zero))
                break;
            --k;
        }
        top = k + 1;
        const Key kept = keys[top];
        keys[top] = keys[i];
        keys[i] = kept;
    }
    for (x86::sreg32 i = 0; i < count; ++i)
        rc::st32(points + i * 4, keys[i].point);
    result = top;
    return true;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    const x86::reg32 comparator = argument(cpu, 2);
    if ((comparator != 0x60ce60 && comparator != 0x60cec0) || !fast::enabled(s_on, "shadow_hull_native")) [[unlikely]]
    {
        sub_60cee0_guest(app, cpu);
        return;
    }
    const x86::reg32 points = argument(cpu, 0);
    const x86::sreg32 count = x86::sreg32(argument(cpu, 1));
    const bool reversed = comparator != 0x60ce60;
    x86::sreg32 top;
    if (count < 2 || count > MostKeys || !hullKeyed(points, count, reversed, top))
        top = hullInPlace(points, count, reversed);
    cpu.eax = x86::reg32(top);
    cpu.esp += 4;
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    const x86::sreg32 count = x86::sreg32(argument(cpu, 1));
    if (count > 0)
        spans.push_back(shadow::Span{argument(cpu, 0), x86::reg32(count) * 4, false});
}

const shadow::Spec spec = {"sub_60cee0", shadow::Eax | shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi | shadow::Esp, 0, false, false};

}

void sub_60cee0(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_60cee0_guest, declare);
}

}}
