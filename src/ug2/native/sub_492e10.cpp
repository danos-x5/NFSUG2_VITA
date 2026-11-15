// Native sub_492e10: particle system update (thiscall: time step, ret 4): emitter timers, then every particle's
// drag, gravity, wind and age, with dead particles and empty groups freed. Small callees are inline.

#include <native/common.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_492e10_guest(win32::WinApplication* app, x86::CPU& cpu);
void sub_43bdf0(win32::WinApplication* app, x86::CPU& cpu);
void sub_491e50(win32::WinApplication* app, x86::CPU& cpu);
void sub_491c50(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::reg32;
using x86::sreg32;

inline reg32 ftol2(Real value) { return reg32(x86::truncInteger64(double(value))); }

inline Real integer(reg32 value) { return Real(sreg32(value)); }

inline Real rounded(Real value) { return Real(float(value)); }

#if defined(__ARM_FP) && !defined(NFSU_FPU_DOUBLE)

// One instruction each, operands in the x87 order: a NaN keeps its sign and the first of two NaNs wins.
#ifdef __aarch64__
#define UG2_FLOAT_OP(name, a64, a32) \
    inline float name(float a, float b) { float r; __asm__(a64 " %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
#else
#define UG2_FLOAT_OP(name, a64, a32) \
    inline float name(float a, float b) { float r; __asm__(a32 " %0, %1, %2" : "=t"(r) : "t"(a), "t"(b)); return r; }
#endif
UG2_FLOAT_OP(add, "fadd", "vadd.f32")
UG2_FLOAT_OP(sub, "fsub", "vsub.f32")
UG2_FLOAT_OP(mul, "fmul", "vmul.f32")
UG2_FLOAT_OP(div, "fdiv", "vdiv.f32")
#undef UG2_FLOAT_OP

#else

inline Real add(Real a, Real b) { return a + b; }
inline Real sub(Real a, Real b) { return a - b; }
inline Real mul(Real a, Real b) { return a * b; }
inline Real div(Real a, Real b) { return a / b; }

#endif

// Pool free of sub_43f920.
inline void release(reg32 pool, reg32 object)
{
    const reg32 count = rc::ld32(pool + 0x18);
    rc::st32(object, rc::ld32(pool + 0x10));
    rc::st32(pool + 0x18, count - 1);
    rc::st32(pool + 0x10, object);
}

// Camera position copy of sub_48bd90.
inline void copyCamera(reg32 self, reg32 slot, reg32 offset)
{
    const reg32 camera = rc::ld32(slot);
    if (camera == 0)
        return;
    const reg32 x = rc::ld32(camera + 0x40), y = rc::ld32(camera + 0x44), z = rc::ld32(camera + 0x48);
    rc::st32(self + offset, x);
    rc::st32(self + offset + 4, y);
    rc::st32(self + offset + 8, z);
}

// Guest callee with the guest's callee-saved registers.
inline bool call(win32::WinApplication* app, x86::CPU& cpu, void (*child)(win32::WinApplication*, x86::CPU&),
                 reg32 esp, reg32 ecx, reg32 ebx, reg32 esi, reg32 edi)
{
    cpu.ecx = ecx;
    cpu.ebx = ebx;
    cpu.esi = esi;
    cpu.edi = edi;
    cpu.esp = esp;
    child(app, cpu);
    return !cpu.terminate;
}

struct Pass
{
    reg32 mask;
    sreg32 step;
    Real dt, drag, gravity, zero;
    Real wind[3];
    bool windOn;
};

// One live particle: speed from bytes, drag, gravity, wind, position, speed back to bytes.
inline void advance(x86::CPU& cpu, reg32 p, sreg32 life, const Pass& s)
{
    reg32 frame = rc::ld8(p + 0x1f);
    if (s.step != 0)
    {
        const reg32 after = rc::divideSigned(reg32(life >> 31), reg32(life), reg32(s.step)).quotient;
        const sreg32 age = x86::sreg16(rc::ld16(p + 8));
        const reg32 before = rc::divideSigned(reg32(age >> 31), reg32(age), reg32(s.step)).quotient;
        frame = (before - after + frame) & s.mask;
    }
    const Real weight = mul(integer(rc::ld8(p + 0xe)), ld(0x784548));
    const Real speed = mul(integer(rc::ld16(p + 0xc)), ld(0x787af0));
    const Real unit = ld(0x78a94c);
    Real vx = mul(mul(integer(reg32(x86::sreg8(rc::ld8(p + 0x1c)))), unit), speed);
    Real vy = mul(mul(integer(reg32(x86::sreg8(rc::ld8(p + 0x1d)))), unit), speed);
    Real vz = mul(speed, mul(integer(reg32(x86::sreg8(rc::ld8(p + 0x1e)))), unit));
    const Real px = ld(p + 0x10);
    const Real py = ld(p + 0x14);
    const Real pz = ld(p + 0x18);
    const Real signX = vx < s.zero ? Real(1.0f) : Real(-1.0f);
    const Real signY = vy < s.zero ? Real(1.0f) : Real(-1.0f);
    const Real signZ = vz < s.zero ? ld(0x784250) : ld(0x7842a0);

    vx = add(mul(mul(mul(mul(vx, vx), s.drag), s.dt), signX), vx);
    vy = add(mul(mul(mul(mul(vy, vy), s.drag), s.dt), signY), vy);
    vz = add(vz, mul(add(mul(mul(mul(signZ, vz), vz), s.drag), s.gravity), s.dt));
    Real nx = add(mul(vx, s.dt), px);
    Real ny = add(mul(vy, s.dt), py);
    Real nz = add(mul(vz, s.dt), pz);
    if (s.windOn)
    {
        nx = add(nx, mul(weight, s.wind[0]));
        ny = add(ny, mul(weight, s.wind[1]));
        nz = add(mul(weight, s.wind[2]), nz);
    }
    Real length = add(mul(vz, vz), mul(vy, vy));
    length = cpu.fpu.sqrt(add(length, mul(vx, vx)));
    reg32 inverseBits = 0x501502f9u;
    if (double(cpu.fpu.abs(length)) > double(Real(rc::ldf64(0x78da38))))
        inverseBits = bits(float(div(ld(0x784250), length)));
    float inverseFloat;
    memcpy(&inverseFloat, &inverseBits, 4);
    const Real inverse = Real(inverseFloat);

    rc::st16(p + 8, x86::reg16(life));
    const reg32 newSpeed = ftol2(mul(length, ld(0x784540)));
    rc::stf32(p + 0x10, float(nx));
    rc::st16(p + 0xc, x86::reg16(newSpeed));
    rc::stf32(p + 0x14, float(ny));
    rc::stf32(p + 0x18, float(nz));
    const Real back = ld(0x78b104);
    rc::st8(p + 0x1c, x86::reg8(ftol2(mul(mul(inverse, vx), back))));
    rc::st8(p + 0x1d, x86::reg8(ftol2(mul(mul(inverse, vy), back))));
    const x86::reg8 last = x86::reg8(ftol2(mul(mul(inverse, vz), back)));
    rc::st8(p + 0x1f, x86::reg8(frame));
    rc::st8(p + 0x1e, last);
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("particle_update_native", true); }))
    {
        sub_492e10_guest(app, cpu);
        return;
    }
    const reg32 entry = cpu.esp;
    const reg32 savedEbx = cpu.ebx, savedEbp = cpu.ebp, savedEsi = cpu.esi, savedEdi = cpu.edi;
    const reg32 self = cpu.ecx;
    rc::st32(entry - 4, savedEbp);
    cpu.ebp = entry - 4;
    const reg32 frame = ((entry - 4) & 0xfffffff0u) - 0x64 - 12;
    const reg32 dtBits = rc::ld32(entry + 4);
    const Real dt = ld(entry + 4);

    if (!call(app, cpu, sub_43bdf0, frame - 4, self, savedEbx, savedEsi, self))
        return;
    const reg32 ticks = ftol2(mul(dt, ld(0x78a194)));
    copyCamera(self, 0x832e90, 0x10);
    copyCamera(self, 0x832f00, 0x20);
    copyCamera(self, 0x832e20, 0x30);

    reg32 state = savedEbx;
    for (reg32 node = rc::ld32(self + 8); node != self + 8;)
    {
        const reg32 next = rc::ld32(node);
        bool stop = false;
        const reg32 key = rc::ld32(node + 0x2c);
        bool active = true;
        if (key != 0)
        {
            active = false;
            for (int i = 0; i < 8; ++i)
            {
                if (rc::ld32(0x883e70 + 0x641c + 4 * i) == key)
                {
                    active = true;
                    break;
                }
            }
        }
        if (active)
        {
            state = rc::ld32(node + 0x10);
            if (state == 1)
            {
                rc::st32(frame - 4, 0);
                rc::st32(frame - 8, dtBits);
                rc::st32(frame - 12, node);
                if (!call(app, cpu, sub_491e50, frame - 16, self, state, node, self))
                    return;
            }
            else if (state != 0)
            {
                stop = true;
            }
            const Real zero = ld(0x7d7d3c);
            const Real timer = ld(node + 0x14);
            if (!(timer == zero) && (state == 1 || state == 0))
            {
                const Real left = sub(timer, dt);
                if (left <= zero)
                {
                    const reg32 period = rc::ld32(node + 0x1c);
                    Real base, spread;
                    if (state == 0)
                    {
                        base = ld(period + 0x24);
                        spread = ld(period + 0x28);
                        state = 1;
                    }
                    else
                    {
                        base = ld(period + 0x2c);
                        spread = ld(period + 0x30);
                        state = 0;
                    }
                    if (base == zero)
                    {
                        rc::st32(node + 0x10, state);
                        rc::stf32(node + 0x14, float(base));
                        rc::st32(node + 0x18, 0);
                    }
                    else
                    {
                        const Real range = mul(spread, base);
                        reg32 seed = rc::ld32(node + 0x24);
                        seed ^= seed << 17;
                        seed ^= seed >> 13;
                        seed ^= seed << 5;
                        rc::st32(node + 0x24, seed);
                        const Real random = mul(mul(integer(seed & 0x7fffffffu), ld(0x78d864)), range);
                        rc::st32(node + 0x10, state);
                        rc::st32(node + 0x18, 0);
                        rc::stf32(node + 0x14, float(add(add(random, sub(base, range)), left)));
                    }
                }
                else
                {
                    rc::stf32(node + 0x14, float(left));
                }
            }
        }
        node = next;
        if (stop)
            break;
        app->allowPreemption(cpu);
    }

    for (reg32 group = rc::ld32(self); group != self;)
    {
        const reg32 nextGroup = rc::ld32(group);
        for (int pass = 0; pass < 2; ++pass)
        {
            const reg32 head = group + 0x14 + 8 * pass;
            const reg32 desc = rc::ld32(group + 8);
            Pass s;
            s.mask = (1u << (rc::ld8(desc + 0x1a) & 31)) * (1u << (rc::ld8(desc + 0x19) & 31)) - 1;
            const x86::sreg8 rate = x86::sreg8(rc::ld8(desc + 0x1b));
            s.step = rate != 0 ? sreg32(ftol2(div(ld(0x78a194), integer(reg32(sreg32(rate)))))) : 0;
            s.gravity = rounded(-ld(desc + 0x84));
            s.drag = ld(desc + 0x88);
            s.dt = dt;
            s.zero = ld(0x7d7d3c);
            s.wind[0] = s.wind[1] = s.wind[2] = Real(0.0f);
            s.windOn = rc::ld8(desc + 0x1e) != 0;
            if (s.windOn)
            {
                const reg32 emitter = rc::ld32(group + 0x10);
                if (emitter != 0 && ld(desc + 0x54) > s.zero)
                {
                    s.wind[0] = mul(dt, ld(emitter + 0x70));
                    s.wind[1] = mul(dt, ld(emitter + 0x74));
                    s.wind[2] = mul(dt, ld(emitter + 0x78));
                }
            }
            for (reg32 p = rc::ld32(head); p != head;)
            {
                const sreg32 life = sreg32(x86::sreg16(rc::ld16(p + 8))) - sreg32(ticks);
                const reg32 next = rc::ld32(p);
                if (life > 0)
                {
                    advance(cpu, p, life, s);
                }
                else
                {
                    const reg32 previous = rc::ld32(p + 4);
                    rc::st32(previous, next);
                    rc::st32(next + 4, previous);
                    const reg32 counter = group + 0x24 + 4 * pass;
                    rc::st32(counter, rc::ld32(counter) - 1);
                    release(rc::ld32(0x82da38), p);
                }
                p = next;
                app->allowPreemption(cpu);
            }
        }
        const reg32 second = rc::ld32(group + 0x1c);
        if (rc::ld32(group + 0x14) == group + 0x14 && second == group + 0x1c)
        {
            const reg32 previous = rc::ld32(group + 4);
            const reg32 next = rc::ld32(group);
            rc::st32(previous, next);
            rc::st32(next + 4, previous);
            if (!call(app, cpu, sub_491c50, frame - 4, group, group, 0, second))
                return;
            release(rc::ld32(0x82da3c), group);
        }
        group = nextGroup;
        app->allowPreemption(cpu);
    }

    cpu.ebx = savedEbx;
    cpu.ebp = savedEbp;
    cpu.esi = savedEsi;
    cpu.edi = savedEdi;
    cpu.esp = entry + 8;
}

}

// No shadow compare: it frees particles, draws random numbers and emits: a second run would change the game.
void sub_492e10(win32::WinApplication* app, x86::CPU& cpu)
{
    body(app, cpu);
}

}}
