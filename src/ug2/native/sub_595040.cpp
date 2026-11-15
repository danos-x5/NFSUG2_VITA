// Native sub_595040: closest features of two simplices (GJK-style search). Guest frame in locals, callees inlined, float ops in the guest build's operand order.

#include <native/common.h>
#include <native/shadow.h>
#include <platform/cached.h>
#include <platform/config.h>

namespace ug2
{

void sub_595040_guest(win32::WinApplication* app, x86::CPU& cpu);

namespace native
{

namespace
{

using x86::Float;
using x86::reg32;

inline Float fl(reg32 v)
{
    Float f;
    memcpy(&f, &v, 4);
    return f;
}

// Fixed operand order, so NaN results match the guest build.
#if defined(__aarch64__)
#define PINNED_OP(name, ins)                                                 \
    inline Float name(Float a, Float b)                                      \
    {                                                                        \
        Float r;                                                             \
        asm(ins " %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b));                \
        return r;                                                            \
    }
PINNED_OP(pmul, "fmul")
PINNED_OP(padd, "fadd")
PINNED_OP(psub, "fsub")
PINNED_OP(pdiv, "fdiv")

inline Float pneg(Float a)
{
    Float r;
    asm("fneg %s0, %s1" : "=w"(r) : "w"(a));
    return r;
}
#elif defined(__arm__)
#define PINNED_OP(name, ins)                                                 \
    inline Float name(Float a, Float b)                                      \
    {                                                                        \
        Float r;                                                             \
        asm(ins ".f32 %0, %1, %2" : "=t"(r) : "t"(a), "t"(b));               \
        return r;                                                            \
    }
PINNED_OP(pmul, "vmul")
PINNED_OP(padd, "vadd")
PINNED_OP(psub, "vsub")
PINNED_OP(pdiv, "vdiv")

inline Float pneg(Float a)
{
    Float r;
    asm("vneg.f32 %0, %1" : "=t"(r) : "t"(a));
    return r;
}
#else
inline Float pmul(Float a, Float b) { return a * b; }
inline Float padd(Float a, Float b) { return a + b; }
inline Float psub(Float a, Float b) { return a - b; }
inline Float pdiv(Float a, Float b) { return a / b; }
inline Float pneg(Float a) { return -a; }
#endif
#undef PINNED_OP

void finish(x86::CPU& cpu, reg32 fpu_count, Float result)
{
    cpu.esp += 8;
    cpu.fpu.regs[(fpu_count + 1) & 7] = result;
    cpu.fpu.count = fpu_count + 1;
}

void search(win32::WinApplication* app, x86::CPU& cpu)
{
  RC_USE(cpu);
  RC_USE(app);
  [[maybe_unused]] x86::reg32 ebx = cpu.ebx;
  [[maybe_unused]] x86::reg32 esi = cpu.esi;
  [[maybe_unused]] x86::reg32 edi = cpu.edi;
  const x86::reg32 fpu_count = cpu.fpu.count;
  [[maybe_unused]] x86::Float fp1 = 0;
  [[maybe_unused]] x86::Float fp2 = 0;
  [[maybe_unused]] x86::Float fp3 = 0;
  [[maybe_unused]] x86::Float fp4 = 0;
  [[maybe_unused]] x86::Float fp5 = 0;
  x86::reg32 lza_595062 = 0;
  x86::reg32 lza_595082 = 0;
  x86::reg32 lza_595128 = 0;
  x86::reg32 lza_595182 = 0;
  x86::reg32 lza_5952f1 = 0;
  x86::reg32 lza_5953f8 = 0;
  x86::reg32 lza_595404 = 0;
  x86::reg32 lza_595440 = 0;
  x86::reg32 lza_5954ec = 0;
  x86::reg32 lza_595536 = 0;
  x86::reg32 lza_595652 = 0;
  x86::reg32 lza_595671 = 0;
  x86::reg32 lza_5956ad = 0;
  x86::reg32 lza_5956e5 = 0;
  x86::reg32 lza_595936 = 0;
  x86::reg32 lza_59599d = 0;
  x86::reg32 lza_595adc = 0;
  x86::reg32 lza_595bd0 = 0;
  x86::reg32 lza_595bdc = 0;
  x86::reg32 lza_595c1f = 0;
  x86::reg32 lza_595cd4 = 0;
  x86::reg32 lza_595d22 = 0;
  x86::reg32 lza_595e30 = 0;
  x86::reg32 lza_595e51 = 0;
  x86::reg32 lza_595f57 = 0;
  x86::reg32 lza_595f70 = 0;
  x86::reg32 lza_59605a = 0;
  x86::reg32 lza_596883 = 0;
  x86::reg32 lza_596975 = 0;
  x86::reg32 lza_596981 = 0;
  x86::reg32 lza_5969bf = 0;
  x86::reg32 lza_5969ca = 0;
  x86::reg32 lza_596abc = 0;
  x86::reg32 lza_596ac8 = 0;
  x86::reg32 lza_596b04 = 0;
  x86::reg32 lza_596c4e = 0;
  x86::reg32 lza_596c56 = 0;
  x86::reg32 lza_596c62 = 0;
  x86::reg32 lza_596c6a = 0;
  x86::reg32 lza_596dca = 0;
  x86::reg32 lza_596dda = 0;
  x86::reg32 lza_596df1 = 0;
  x86::reg32 lza_596e01 = 0;
  x86::reg32 lza_597000 = 0;
  x86::reg32 lza_59704e = 0;
  x86::reg32 lza_597141 = 0;
  x86::reg32 lza_597151 = 0;
  x86::reg32 lza_59718f = 0;
  x86::reg32 lza_59747d = 0;
  x86::reg32 lza_5975a8 = 0;
  x86::reg32 lza_5975e1 = 0;
  x86::reg32 lza_597613 = 0;
  x86::reg32 lza_597655 = 0;
  x86::reg32 lza_597687 = 0;
  x86::reg32 lza_5978c7 = 0;
  x86::reg32 lza_597914 = 0;
  x86::reg32 lza_597a0b = 0;
  x86::reg32 lza_597a1e = 0;
  x86::reg32 lza_597a5f = 0;
  x86::reg32 lza_597d20 = 0;
  x86::reg32 lza_597e80 = 0;
  x86::reg32 lza_597eb6 = 0;
  x86::reg32 lza_597ee5 = 0;
  x86::reg32 lza_597fd7 = 0;
  x86::reg32 lza_59802b = 0;
  x86::reg32 lzb_595062 = 0;
  x86::reg32 lzb_595082 = 0;
  x86::reg32 lzb_595128 = 0;
  x86::reg32 lzb_595182 = 0;
  x86::reg32 lzb_5952f1 = 0;
  x86::reg32 lzb_5953f8 = 0;
  x86::reg32 lzb_595404 = 0;
  x86::reg32 lzb_595440 = 0;
  x86::reg32 lzb_5954ec = 0;
  x86::reg32 lzb_595536 = 0;
  x86::reg32 lzb_595652 = 0;
  x86::reg32 lzb_595671 = 0;
  x86::reg32 lzb_5956ad = 0;
  x86::reg32 lzb_5956e5 = 0;
  x86::reg32 lzb_595936 = 0;
  x86::reg32 lzb_59599d = 0;
  x86::reg32 lzb_595adc = 0;
  x86::reg32 lzb_595bd0 = 0;
  x86::reg32 lzb_595bdc = 0;
  x86::reg32 lzb_595c1f = 0;
  x86::reg32 lzb_595cd4 = 0;
  x86::reg32 lzb_595d22 = 0;
  x86::reg32 lzb_595e30 = 0;
  x86::reg32 lzb_595e51 = 0;
  x86::reg32 lzb_595f57 = 0;
  x86::reg32 lzb_595f70 = 0;
  x86::reg32 lzb_59605a = 0;
  x86::reg32 lzb_596883 = 0;
  x86::reg32 lzb_596975 = 0;
  x86::reg32 lzb_596981 = 0;
  x86::reg32 lzb_5969bf = 0;
  x86::reg32 lzb_5969ca = 0;
  x86::reg32 lzb_596abc = 0;
  x86::reg32 lzb_596ac8 = 0;
  x86::reg32 lzb_596b04 = 0;
  x86::reg32 lzb_596c4e = 0;
  x86::reg32 lzb_596c56 = 0;
  x86::reg32 lzb_596c62 = 0;
  x86::reg32 lzb_596c6a = 0;
  x86::reg32 lzb_596dca = 0;
  x86::reg32 lzb_596dda = 0;
  x86::reg32 lzb_596df1 = 0;
  x86::reg32 lzb_596e01 = 0;
  x86::reg32 lzb_597000 = 0;
  x86::reg32 lzb_59704e = 0;
  x86::reg32 lzb_597141 = 0;
  x86::reg32 lzb_597151 = 0;
  x86::reg32 lzb_59718f = 0;
  x86::reg32 lzb_59747d = 0;
  x86::reg32 lzb_5975a8 = 0;
  x86::reg32 lzb_5975e1 = 0;
  x86::reg32 lzb_597613 = 0;
  x86::reg32 lzb_597655 = 0;
  x86::reg32 lzb_597687 = 0;
  x86::reg32 lzb_5978c7 = 0;
  x86::reg32 lzb_597914 = 0;
  x86::reg32 lzb_597a0b = 0;
  x86::reg32 lzb_597a1e = 0;
  x86::reg32 lzb_597a5f = 0;
  x86::reg32 lzb_597d20 = 0;
  x86::reg32 lzb_597e80 = 0;
  x86::reg32 lzb_597eb6 = 0;
  x86::reg32 lzb_597ee5 = 0;
  x86::reg32 lzb_597fd7 = 0;
  x86::reg32 lzb_59802b = 0;
  double lzfa_595249 = 0;
  double lzfa_59528c = 0;
  double lzfa_595352 = 0;
  double lzfa_59537e = 0;
  double lzfa_5953ad = 0;
  double lzfa_5953dc = 0;
  double lzfa_59542d = 0;
  double lzfa_595544 = 0;
  double lzfa_59560e = 0;
  double lzfa_595863 = 0;
  double lzfa_59588d = 0;
  double lzfa_595a3b = 0;
  double lzfa_595a7f = 0;
  double lzfa_595b2b = 0;
  double lzfa_595b56 = 0;
  double lzfa_595b85 = 0;
  double lzfa_595bb4 = 0;
  double lzfa_595c0c = 0;
  double lzfa_595d30 = 0;
  double lzfa_595dda = 0;
  double lzfa_595e72 = 0;
  double lzfa_595eba = 0;
  double lzfa_59619c = 0;
  double lzfa_5961b5 = 0;
  double lzfa_5961eb = 0;
  double lzfa_596218 = 0;
  double lzfa_59622d = 0;
  double lzfa_596242 = 0;
  double lzfa_596257 = 0;
  double lzfa_5962a6 = 0;
  double lzfa_596465 = 0;
  double lzfa_5964a4 = 0;
  double lzfa_5964f4 = 0;
  double lzfa_596537 = 0;
  double lzfa_59654c = 0;
  double lzfa_5965b9 = 0;
  double lzfa_59660e = 0;
  double lzfa_59672a = 0;
  double lzfa_596759 = 0;
  double lzfa_59676e = 0;
  double lzfa_596804 = 0;
  double lzfa_5968d0 = 0;
  double lzfa_5968fb = 0;
  double lzfa_59692a = 0;
  double lzfa_596959 = 0;
  double lzfa_5969ac = 0;
  double lzfa_596a16 = 0;
  double lzfa_596a42 = 0;
  double lzfa_596a71 = 0;
  double lzfa_596aa0 = 0;
  double lzfa_596af1 = 0;
  double lzfa_596c7a = 0;
  double lzfa_596d24 = 0;
  double lzfa_596d5e = 0;
  double lzfa_596d83 = 0;
  double lzfa_596e29 = 0;
  double lzfa_596e69 = 0;
  double lzfa_596efd = 0;
  double lzfa_596f46 = 0;
  double lzfa_597034 = 0;
  double lzfa_59709b = 0;
  double lzfa_5970c7 = 0;
  double lzfa_5970f6 = 0;
  double lzfa_597125 = 0;
  double lzfa_59717c = 0;
  double lzfa_59726a = 0;
  double lzfa_597329 = 0;
  double lzfa_5973e8 = 0;
  double lzfa_5974c3 = 0;
  double lzfa_5978fc = 0;
  double lzfa_597966 = 0;
  double lzfa_597991 = 0;
  double lzfa_5979c0 = 0;
  double lzfa_5979ef = 0;
  double lzfa_597a4c = 0;
  double lzfa_597b3c = 0;
  double lzfa_597bfb = 0;
  double lzfa_597cba = 0;
  double lzfa_597d98 = 0;
  double lzfb_595249 = 0;
  double lzfb_59528c = 0;
  double lzfb_595352 = 0;
  double lzfb_59537e = 0;
  double lzfb_5953ad = 0;
  double lzfb_5953dc = 0;
  double lzfb_59542d = 0;
  double lzfb_595544 = 0;
  double lzfb_59560e = 0;
  double lzfb_595863 = 0;
  double lzfb_59588d = 0;
  double lzfb_595a3b = 0;
  double lzfb_595a7f = 0;
  double lzfb_595b2b = 0;
  double lzfb_595b56 = 0;
  double lzfb_595b85 = 0;
  double lzfb_595bb4 = 0;
  double lzfb_595c0c = 0;
  double lzfb_595d30 = 0;
  double lzfb_595dda = 0;
  double lzfb_595e72 = 0;
  double lzfb_595eba = 0;
  double lzfb_59619c = 0;
  double lzfb_5961b5 = 0;
  double lzfb_5961eb = 0;
  double lzfb_596218 = 0;
  double lzfb_59622d = 0;
  double lzfb_596242 = 0;
  double lzfb_596257 = 0;
  double lzfb_5962a6 = 0;
  double lzfb_596465 = 0;
  double lzfb_5964a4 = 0;
  double lzfb_5964f4 = 0;
  double lzfb_596537 = 0;
  double lzfb_59654c = 0;
  double lzfb_5965b9 = 0;
  double lzfb_59660e = 0;
  double lzfb_59672a = 0;
  double lzfb_596759 = 0;
  double lzfb_59676e = 0;
  double lzfb_596804 = 0;
  double lzfb_5968d0 = 0;
  double lzfb_5968fb = 0;
  double lzfb_59692a = 0;
  double lzfb_596959 = 0;
  double lzfb_5969ac = 0;
  double lzfb_596a16 = 0;
  double lzfb_596a42 = 0;
  double lzfb_596a71 = 0;
  double lzfb_596aa0 = 0;
  double lzfb_596af1 = 0;
  double lzfb_596c7a = 0;
  double lzfb_596d24 = 0;
  double lzfb_596d5e = 0;
  double lzfb_596d83 = 0;
  double lzfb_596e29 = 0;
  double lzfb_596e69 = 0;
  double lzfb_596efd = 0;
  double lzfb_596f46 = 0;
  double lzfb_597034 = 0;
  double lzfb_59709b = 0;
  double lzfb_5970c7 = 0;
  double lzfb_5970f6 = 0;
  double lzfb_597125 = 0;
  double lzfb_59717c = 0;
  double lzfb_59726a = 0;
  double lzfb_597329 = 0;
  double lzfb_5973e8 = 0;
  double lzfb_5974c3 = 0;
  double lzfb_5978fc = 0;
  double lzfb_597966 = 0;
  double lzfb_597991 = 0;
  double lzfb_5979c0 = 0;
  double lzfb_5979ef = 0;
  double lzfb_597a4c = 0;
  double lzfb_597b3c = 0;
  double lzfb_597bfb = 0;
  double lzfb_597cba = 0;
  double lzfb_597d98 = 0;
  x86::reg32 lzr_59544a = 0;
  x86::reg32 lzr_5955e9 = 0;
  x86::reg32 lzr_595688 = 0;
  x86::reg32 lzr_595817 = 0;
  x86::reg32 lzr_595944 = 0;
  x86::reg32 lzr_595c3e = 0;
  x86::reg32 lzr_595dc0 = 0;
  x86::reg32 lzr_595fbd = 0;
  x86::reg32 lzr_5961d0 = 0;
  x86::reg32 lzr_596621 = 0;
  x86::reg32 lzr_596b0e = 0;
  x86::reg32 lzr_596d0a = 0;
  x86::reg32 lzr_59719d = 0;
  x86::reg32 lzr_59762d = 0;
  x86::reg32 lzr_5977b4 = 0;
  x86::reg32 lzr_597a72 = 0;
  x86::reg32 lzr_597efc = 0;
  x86::reg32 lzr_597f7f = 0;
  x86::reg32 lzr_597fe4 = 0;
  double lzfa_43cf3c = 0;
  double lzfb_43cf3c = 0;
  [[maybe_unused]] x86::reg32 eax = 0, ecx = 0, edx = 0;
  double cmpa = 0, cmpb = 0;
  const Float kOne = Float(rc::ldf32(0x784250u));
  const Float kZero = Float(rc::ldf32(0x7d7d3cu));
  const Float kEps = Float(rc::ldf32(0x7a1d40u));
  reg32 s_m28, s_m24, s_m20, s_m1c, s_m18, s_m14, s_m10, s_mc, s_m4, s_014, s_018, s_01c, s_020, s_024, s_028, s_02c, s_030, s_034, s_038, s_03c, s_040, s_044, s_048, s_04c, s_050, s_054, s_058, s_064, s_068, s_06c, s_070, s_074, s_078, s_088, s_08c, s_090, s_094, s_098, s_0a0, s_0a4, s_0a8, s_0b0, s_0b4, s_0b8, s_0cc, s_0d0, s_0d4, s_0d8, s_0e0, s_0e4, s_0e8, s_0f0, s_0f4, s_0f8, s_100, s_104, s_108, s_128, s_13c, s_140, s_144, s_148, s_150, s_154, s_158, s_160, s_164, s_168, s_17c, s_180, s_184, s_188, s_190, s_194, s_198, s_1a0, s_1a4, s_1a8, s_1bc, s_1c0, s_1c4, s_1c8, s_1d0, s_1d4, s_1d8, s_1e8, s_1f0, s_1f8, s_208, s_218, s_228, s_238, s_248, s_250, s_258, s_268, s_278, s_288, s_290, s_298, s_2a8, s_2b8;
  reg32 IA[4], IB[4];
  esi = argument(cpu, 0);
  edi = cpu.ecx;
  eax = rc::ld32(edi + 0xc0u);
  ecx = rc::ld32(edi + 0xd4u);
  edx = 0;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x1u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595062 = a;
  lzb_595062 = b;
  }
  s_0cc = edx;
  s_068 = 0x7f7fffffu;
  s_040 = eax;
  s_04c = ecx;
  if ((x86::reg32(lza_595062) != x86::reg32(lzb_595062))) goto L_0x00595936;
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x1u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595082 = a;
  lzb_595082 = b;
  }
  if ((x86::reg32(lza_595082) != x86::reg32(lzb_595082))) goto L_0x00595127;
  fp1 = x86::Float(rc::ldf32(edi + 0x4u));
  eax = rc::ld32(edi);
  fp2 = x86::Float(rc::ldf32(edi + 0x8u));
  rc::st32(edi + 0x80u, eax);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  eax = esi;
  rc::stf32(edi + 0x84u, float(fp2));
  rc::stf32(edi + 0x88u, float(fp1));
  ecx = rc::ld32(edi + 0x40u);
  fp1 = x86::Float(rc::ldf32(edi + 0x44u));
  fp2 = x86::Float(rc::ldf32(edi + 0x48u));
  rc::st32(edi + 0x90u, ecx);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(edi + 0x94u, float(fp2));
  rc::stf32(edi + 0x98u, float(fp1));
  fp1 = x86::Float(rc::ldf32(edi + 0x90u));
  fp1 = psub(fp1, Float(rc::ldf32(edi + 0x80u)));
  fp2 = x86::Float(rc::ldf32(edi + 0x94u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x84u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x98u));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x88u)));
  s_198 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_030 = bits(fp2);
  rc::stf32(eax, float(fp2));
  rc::stf32(eax + 0x4u, float(fp1));
  fp2 = x86::Float(fl(s_198));
  rc::stf32(eax + 0x8u, float(fp2));
  fp3 = fp2;
  fp3 = pmul(fp3, fp2);
  fp4 = fp1;
  fp4 = pmul(fp4, fp1);
  fp3 = padd(fp4, fp3);
  fp4 = x86::Float(fl(s_030));
  fp4 = pmul(fp4, Float(fl(s_030)));
  fp3 = padd(fp3, fp4);
  fp1 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  finish(cpu, fpu_count, fp1);
  return;
L_0x00595127:
  {
  const x86::reg32 r = x86::reg32(ecx - 1);
  ecx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = edx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595128 = a;
  lzb_595128 = b;
  }
  s_03c = edx;
  s_034 = ecx;
  if ((x86::sreg32(x86::reg32(lza_595128)) <= x86::sreg32(x86::reg32(lzb_595128)))) goto L_0x00595685;
  edx = x86::reg32(edi + 0x48u);
  s_024 = edx;
L_0x00595140:
  eax = s_024;
  fp1 = x86::Float(rc::ldf32(eax - 0x8u));
  ecx = s_04c;
  fp1 = psub(fp1, Float(rc::ldf32(edi)));
  fp2 = x86::Float(rc::ldf32(eax - 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(eax));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x8u)));
  s_198 = bits(fp3);
  eax = s_198;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0d8 = eax;
  s_0d0 = bits(fp2);
  eax = s_03c;
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  s_0d4 = bits(fp1);
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595182 = a;
  lzb_595182 = b;
  }
  s_06c = eax;
  s_038 = eax;
  if ((x86::sreg32(x86::reg32(lza_595182)) >= x86::sreg32(x86::reg32(lzb_595182)))) goto L_0x00595662;
  ecx = s_024;
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  s_030 = ecx;
  ecx = x86::reg32(ecx);
L_0x005951a0:
  edx = s_030;
  ecx = s_024;
  fp1 = x86::Float(rc::ldf32(edx - 0x8u));
  fp1 = psub(fp1, Float(rc::ldf32(ecx - 0x8u)));
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = psub(Float(rc::ldf32(edx - 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(ecx));
  fp3 = psub(Float(rc::ldf32(edx)), fp3);
  s_1d8 = bits(fp3);
  eax = s_1d8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_090 = bits(fp2);
  s_098 = eax;
  s_094 = bits(fp1);
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_0d0)));
  fp2 = x86::Float(fl(s_098));
  fp2 = pmul(fp2, Float(fl(s_0d8)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(fp2, Float(fl(s_0d4)));
  fp1 = padd(fp1, fp2);
  fp1 = pneg(fp1);
  fp2 = x86::Float(fl(s_090));
  fp2 = pmul(fp2, Float(fl(s_090)));
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(fp3, Float(fl(s_098)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_094));
  fp3 = pmul(fp3, Float(fl(s_094)));
  fp2 = padd(fp3, fp2);
  fp1 = pdiv(fp1, fp2);
  s_020 = bits(fp1);
  lzfa_595249 = fp1;
  lzfb_595249 = kZero;
  if (!(lzfa_595249 < lzfb_595249)) goto L_0x00595280;
  fp1 = x86::Float(rc::ldf32(ecx - 0x8u));
  s_020 = 0x0u;
  fp1 = psub(fp1, Float(rc::ldf32(edi)));
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x8u)));
  s_1a8 = bits(fp3);
  ecx = s_1a8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(esi, float(fp2));
  rc::st32(esi + 0x8u, ecx);
  rc::stf32(esi + 0x4u, float(fp1));
  goto L_0x005952eb;
L_0x00595280:
  fp1 = x86::Float(fl(s_020));
  lzfa_59528c = fp1;
  lzfb_59528c = kOne;
  if ((x86::reg8((!(lzfa_59528c >= lzfb_59528c) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_59528c, lzfb_59528c) ? 0x40 : 0)) != 0)) goto L_0x005952c3;
  fp1 = x86::Float(rc::ldf32(edx - 0x8u));
  s_020 = 0x3f800000u;
  fp1 = psub(fp1, Float(rc::ldf32(edi)));
  fp2 = x86::Float(rc::ldf32(edx - 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(edx));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x8u)));
  s_1c8 = bits(fp3);
  edx = s_1c8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(esi, float(fp2));
  rc::st32(esi + 0x8u, edx);
  rc::stf32(esi + 0x4u, float(fp1));
  goto L_0x005952eb;
L_0x005952c3:
  s_mc = esi;
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_0d8)));
  fp2 = x86::Float(fl(s_098));
  fp2 = pmul(fp2, Float(fl(s_0d0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(Float(fl(s_0d0)), fp2);
  fp3 = x86::Float(fl(s_0d4));
  fp3 = pmul(Float(fl(s_090)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(fp3, Float(fl(s_0d4)));
  fp4 = x86::Float(fl(s_094));
  fp4 = pmul(Float(fl(s_0d8)), fp4);
  eax = s_mc;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_5952d4:
  s_m10 = esi;
  s_m18 = esi;
  ecx = s_m10;
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = pmul(fp1, Float(fl(s_098)));
  fp2 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp2 = pmul(fp2, Float(fl(s_090)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp2 = pmul(Float(fl(s_090)), fp2);
  fp3 = x86::Float(fl(s_094));
  fp3 = pmul(Float(rc::ldf32(ecx)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp3 = pmul(fp3, Float(fl(s_094)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(Float(fl(s_098)), fp4);
  eax = s_m18;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_5952e3:
L_0x005952eb:
  fp1 = x86::Float(rc::ldf32(esi));
  eax = s_034;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5952f1 = a;
  lzb_5952f1 = b;
  }
  fp1 = pmul(fp1, Float(rc::ldf32(edi)));
  fp2 = x86::Float(rc::ldf32(esi + 0x8u));
  s_028 = 0x0u;
  fp2 = pmul(fp2, Float(rc::ldf32(edi + 0x8u)));
  edx = 0x1u;
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(esi + 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(edi + 0x4u)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x40u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(esi + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(edi + 0x44u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(esi + 0x8u));
  fp3 = pmul(fp3, Float(rc::ldf32(edi + 0x48u)));
  fp2 = padd(fp2, fp3);
  if ((x86::sreg32(x86::reg32(lza_5952f1)) < x86::sreg32(x86::reg32(lzb_5952f1)))) goto L_0x00595400;
  ebx = 0x4u;
  ecx = x86::reg32(edi + 0x54u);
L_0x00595336:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(esi + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(ecx)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_595352 = fp3;
  lzfb_595352 = fp2;
  if (!(lzfa_595352 < lzfb_595352)) goto L_0x00595361;
  fp2 = x86::Float(fp2);
  s_028 = edx;
  fp2 = x86::Float(fl(s_014));
L_0x00595361:
  fp3 = x86::Float(rc::ldf32(ecx + 0x14u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0xcu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x10u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_59537e = fp3;
  lzfb_59537e = fp2;
  if (!(lzfa_59537e < lzfb_59537e)) goto L_0x00595390;
  eax = x86::reg32(ebx - 0x2u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_028 = eax;
L_0x00595390:
  fp3 = x86::Float(rc::ldf32(ecx + 0x24u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x1cu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x20u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_5953ad = fp3;
  lzfb_5953ad = fp2;
  if (!(lzfa_5953ad < lzfb_5953ad)) goto L_0x005953bf;
  eax = x86::reg32(ebx - 0x1u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_028 = eax;
L_0x005953bf:
  fp3 = x86::Float(rc::ldf32(ecx + 0x34u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x2cu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x30u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_5953dc = fp3;
  lzfb_5953dc = fp2;
  if (!(lzfa_5953dc < lzfb_5953dc)) goto L_0x005953eb;
  fp2 = x86::Float(fp2);
  s_028 = ebx;
  fp2 = x86::Float(fl(s_014));
L_0x005953eb:
  eax = s_04c;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x40u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5953f8 = a;
  lzb_5953f8 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_5953f8)) < x86::sreg32(x86::reg32(lzb_5953f8)))) goto L_0x00595336;
L_0x00595400:
  ebx = s_04c;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595404 = a;
  lzb_595404 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_595404)) >= x86::sreg32(x86::reg32(lzb_595404)))) goto L_0x00595444;
  ecx = edx;
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  ecx = x86::reg32(ecx + edi + 0x44u);
L_0x00595411:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(esi + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(ecx)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_59542d = fp3;
  lzfb_59542d = fp2;
  if (!(lzfa_59542d < lzfb_59542d)) goto L_0x0059543c;
  fp2 = x86::Float(fp2);
  s_028 = edx;
  fp2 = x86::Float(fl(s_014));
L_0x0059543c:
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595440 = a;
  lzb_595440 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_595440)) < x86::sreg32(x86::reg32(lzb_595440)))) goto L_0x00595411;
L_0x00595444:
  cmpa = fp2;
  cmpb = fp1;
  fp1 = x86::Float(fp1);
  if (!(cmpa > cmpb)) goto L_0x00595646;
  fp1 = x86::Float(rc::ldf32(edi + 0x4u));
  edx = rc::ld32(edi);
  fp2 = x86::Float(rc::ldf32(edi + 0x8u));
  eax = s_024;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::st32(edi + 0x80u, edx);
  rc::stf32(edi + 0x84u, float(fp2));
  edx = s_03c;
  ecx = s_038;
  rc::stf32(edi + 0x88u, float(fp1));
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_020)));
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(Float(fl(s_020)), fp2);
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(Float(fl(s_020)), fp3);
  s_058 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0a0 = bits(fp2);
  s_0a4 = bits(fp1);
  fp1 = x86::Float(fl(s_058));
  fp2 = x86::Float(fl(s_0a0));
  fp2 = padd(fp2, Float(rc::ldf32(eax - 0x8u)));
  fp3 = x86::Float(rc::ldf32(eax - 0x4u));
  fp3 = padd(Float(fl(s_0a4)), fp3);
  fp4 = x86::Float(rc::ldf32(eax));
  fp4 = padd(fp1, fp4);
  s_1e8 = bits(fp4);
  eax = s_1e8;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x90u, float(fp3));
  rc::st32(edi + 0x98u, eax);
  eax = s_028;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5954ec = a;
  lzb_5954ec = b;
  }
  rc::stf32(edi + 0x94u, float(fp2));
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x90u));
  fp1 = psub(fp1, Float(rc::ldf32(edi + 0x80u)));
  fp2 = x86::Float(rc::ldf32(edi + 0x94u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x84u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x98u));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x88u)));
  fp4 = fp3;
  fp4 = pmul(fp4, fp3);
  fp5 = fp2;
  fp5 = pmul(fp5, fp2);
  fp4 = padd(fp5, fp4);
  fp5 = fp1;
  fp5 = pmul(fp5, fp1);
  fp4 = padd(fp5, fp4);
  fp1 = x86::Float(fp4);
  fp3 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  if ((x86::reg32(lza_5954ec) == x86::reg32(lzb_5954ec))) goto L_0x00595857;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595536 = a;
  lzb_595536 = b;
  }
  if ((x86::reg32(lza_595536) == x86::reg32(lzb_595536))) goto L_0x00595857;
  lzfa_595544 = fp1;
  lzfb_595544 = x86::Float(fl(s_068));
  if (!(lzfa_595544 < lzfb_595544)) goto L_0x00595642;
  fp2 = x86::Float(rc::ldf32(edi + 0x84u));
  eax = rc::ld32(esi);
  s_164 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x88u));
  s_140 = eax;
  eax = rc::ld32(esi + 0x4u);
  s_168 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x94u));
  s_144 = eax;
  eax = rc::ld32(esi + 0x8u);
  s_154 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x98u));
  s_148 = eax;
  eax = rc::ld32(edi + 0x80u);
  s_158 = bits(fp2);
  s_160 = eax;
  eax = rc::ld32(edi + 0x90u);
  s_068 = bits(fp1);
  fp1 = x86::Float(fl(s_020));
  s_150 = eax;
  cmpa = fp1;
  cmpb = kZero;
  s_0cc = 0x1u;
  s_13c = 0x1u;
  IA[0] = 0x0u;
  if (!(cmpa == cmpb)) goto L_0x00595602;
  s_128 = 0x1u;
  IB[0] = edx;
  goto L_0x0059564a;
L_0x00595602:
  fp1 = x86::Float(fl(s_020));
  lzfa_59560e = fp1;
  lzfb_59560e = kOne;
  if (!(lzfa_59560e == lzfb_59560e)) goto L_0x00595627;
  s_128 = 0x1u;
  IB[0] = ecx;
  goto L_0x0059564a;
L_0x00595627:
  s_128 = 0x2u;
  IB[0] = edx;
  IB[1] = ecx;
  goto L_0x0059564a;
L_0x00595642:
  fp1 = x86::Float(fp1);
  goto L_0x0059564a;
L_0x00595646:
  ecx = s_038;
L_0x0059564a:
  edx = s_030;
  {
  const x86::reg32 r = x86::reg32(ecx + 1);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595652 = a;
  lzb_595652 = b;
  }
  s_038 = ecx;
  s_030 = edx;
  if ((x86::sreg32(x86::reg32(lza_595652)) < x86::sreg32(x86::reg32(lzb_595652)))) goto L_0x005951a0;
L_0x00595662:
  edx = s_024;
  eax = s_06c;
  ecx = s_034;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595671 = a;
  lzb_595671 = b;
  }
  s_03c = eax;
  s_024 = edx;
  if ((x86::sreg32(x86::reg32(lza_595671)) < x86::sreg32(x86::reg32(lzb_595671)))) goto L_0x00595140;
L_0x00595681:
  eax = s_040;
L_0x00595685:
  ecx = x86::reg32(eax - 0x2u);
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a & b);
  lzr_595688 = r;
  }
  s_048 = 0x0u;
  if ((x86::sreg32(x86::reg32(lzr_595688)) <= 0)) goto L_0x00597626;
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  s_02c = edi;
  s_08c = 0x5u;
L_0x005956a8:
  ecx = s_048;
  {
  const x86::reg32 r = x86::reg32(ecx + 1);
  ecx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5956ad = a;
  lzb_5956ad = b;
  }
  s_064 = ecx;
  s_044 = ecx;
  if ((x86::sreg32(x86::reg32(lza_5956ad)) >= x86::sreg32(x86::reg32(lzb_5956ad)))) goto L_0x005975f5;
  edx = s_02c;
  eax = s_08c;
  {
  const x86::reg32 r = x86::reg32(ecx + 1);
  ecx = r;
  }
  s_038 = ecx;
  ecx = s_048;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  eax = r;
  }
  s_034 = edx;
  s_088 = eax;
L_0x005956e1:
  eax = s_038;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = s_040;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5956e5 = a;
  lzb_5956e5 = b;
  }
  s_03c = eax;
  if ((x86::sreg32(x86::reg32(lza_5956e5)) >= x86::sreg32(x86::reg32(lzb_5956e5)))) goto L_0x005975bc;
  ecx = s_038;
  eax = s_088;
  edx = s_064;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  eax = r;
  }
  s_018 = eax;
  eax = s_034;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x18u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  s_030 = eax;
  eax = x86::reg32(edx + ecx * 2 - 0x2u);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  ecx = eax;
  eax = edi;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x64u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  s_020 = eax;
  goto L_0x00595730;
L_0x00595730:
  eax = s_034;
  ebx = s_02c;
  fp1 = x86::Float(rc::ldf32(eax));
  fp1 = psub(fp1, Float(rc::ldf32(ebx)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(Float(rc::ldf32(eax + 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(Float(rc::ldf32(eax + 0x8u)), fp3);
  eax = s_030;
  s_288 = bits(fp3);
  edx = s_288;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0e0 = bits(fp2);
  s_0e8 = edx;
  s_0e4 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(eax - 0x8u));
  fp1 = psub(fp1, Float(rc::ldf32(ebx)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(Float(rc::ldf32(eax - 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(Float(rc::ldf32(eax)), fp3);
  s_268 = bits(fp3);
  eax = s_268;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_188 = eax;
  s_180 = bits(fp2);
  s_184 = bits(fp1);
  fp1 = x86::Float(fl(s_180));
  fp1 = pmul(fp1, Float(fl(s_0e8)));
  fp2 = x86::Float(fl(s_188));
  fp2 = pmul(fp2, Float(fl(s_0e0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_184));
  fp2 = pmul(Float(fl(s_0e0)), fp2);
  fp3 = x86::Float(fl(s_0e4));
  fp3 = pmul(Float(fl(s_180)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_188));
  fp3 = pmul(fp3, Float(fl(s_0e4)));
  fp4 = x86::Float(fl(s_184));
  fp4 = pmul(Float(fl(s_0e8)), fp4);
  fp3 = psub(fp3, fp4);
  s_0b0 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0b4 = bits(fp2);
  s_0b8 = bits(fp1);
L_ret_5957be:
  fp1 = x86::Float(fl(s_0b8));
  fp2 = x86::Float(fl(s_0b4));
  fp3 = x86::Float(fl(s_0b0));
  fp4 = fp3;
  fp4 = pmul(fp4, fp3);
  fp5 = fp2;
  fp5 = pmul(fp5, fp2);
  fp4 = padd(fp5, fp4);
  fp5 = fp1;
  fp5 = pmul(fp5, fp1);
  fp4 = padd(fp5, fp4);
  fp4 = cpu.fpu.sqrt(fp4);
  fp1 = x86::Float(fp4);
  fp3 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  lzfa_43cf3c = fp1;
  lzfb_43cf3c = kZero;
  if ((lzfa_43cf3c == lzfb_43cf3c)) goto L_0x0043cf6c_5957d3;
  fp1 = pdiv(kOne, fp1);
  eax = s_0b8;
  s_m10 = eax;
  fp2 = x86::Float(fl(s_0b4));
  fp3 = fp1;
  fp3 = pmul(fp3, Float(fl(s_0b0)));
  s_0b0 = bits(fp3);
  fp2 = pmul(fp1, fp2);
  s_0b4 = bits(fp2);
  fp2 = x86::Float(fl(s_m10));
  fp2 = pmul(fp1, fp2);
  s_0b8 = bits(fp2);
  fp1 = x86::Float(fp1);
  goto L_ret_5957d3;
L_0x0043cf6c_5957d3:
  fp1 = x86::Float(fp1);
  ecx = 0;
  s_0b0 = 0x3f800000u;
  s_0b4 = ecx;
  s_0b8 = ecx;
L_ret_5957d3:
  fp1 = x86::Float(rc::ldf32(edi + 0x40u));
  fp1 = psub(fp1, Float(rc::ldf32(ebx)));
  fp2 = x86::Float(rc::ldf32(edi + 0x44u));
  fp2 = psub(fp2, Float(rc::ldf32(ebx + 0x4u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x48u));
  fp3 = psub(fp3, Float(rc::ldf32(ebx + 0x8u)));
  fp4 = x86::Float(fl(s_0b8));
  fp4 = pmul(fp4, fp3);
  fp5 = x86::Float(fl(s_0b4));
  fp5 = pmul(fp2, fp5);
  fp4 = padd(fp4, fp5);
  {
  const x86::Float t = fp4;
  fp4 = fp1;
  fp1 = t;
  }
  fp4 = pmul(fp4, Float(fl(s_0b0)));
  fp1 = padd(fp1, fp4);
  {
  const x86::Float t = fp3;
  fp3 = fp1;
  fp1 = t;
  }
  cmpa = fp3;
  cmpb = kZero;
  fp1 = x86::Float(fp2);
  fp1 = x86::Float(fp1);
  if (!(cmpa < cmpb)) goto L_0x00596fc9;
  fp1 = x86::Float(fl(s_0b0));
  fp1 = pneg(fp1);
  fp2 = x86::Float(fl(s_0b4));
  fp2 = pneg(fp2);
  fp3 = x86::Float(fl(s_0b8));
  fp3 = pneg(fp3);
  s_248 = bits(fp3);
  eax = s_248;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(esi, float(fp2));
  rc::stf32(esi + 0x4u, float(fp1));
  goto L_0x00596fe3;
L_0x00595857:
  fp2 = x86::Float(fl(s_020));
  lzfa_595863 = fp2;
  lzfb_595863 = kZero;
  if (!(lzfa_595863 == lzfb_595863)) goto L_0x00595881;
  rc::st32(edi + 0xd4u, 0x1u);
  ecx = rc::ld32(edi + edx * 4 + 0xc4u);
  eax = x86::reg32(edx + 0x4u);
  goto L_0x00596f5f;
L_0x00595881:
  fp2 = x86::Float(fl(s_020));
  lzfa_59588d = fp2;
  lzfb_59588d = kOne;
  if (!(lzfa_59588d == lzfb_59588d)) goto L_0x005958d0;
  rc::st32(edi + 0xd4u, 0x1u);
  eax = rc::ld32(edi + ecx * 4 + 0xc4u);
  rc::st32(edi + 0xc4u, eax);
  eax = x86::reg32(ecx + 0x4u);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  ecx = rc::ld32(eax + edi);
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x40u, ecx);
  rc::stf32(edi + 0x44u, float(fp3));
  rc::stf32(edi + 0x48u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x005958d0:
  rc::st32(edi + 0xd4u, 0x2u);
  eax = rc::ld32(edi + edx * 4 + 0xc4u);
  rc::st32(edi + 0xc4u, eax);
  eax = rc::ld32(edi + ecx * 4 + 0xc4u);
  rc::st32(edi + 0xc8u, eax);
  eax = x86::reg32(edx + 0x4u);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  edx = rc::ld32(eax + edi);
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  eax = x86::reg32(ecx + 0x4u);
L_0x0059590a:
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  rc::stf32(edi + 0x44u, float(fp3));
  rc::st32(edi + 0x40u, edx);
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  rc::stf32(edi + 0x48u, float(fp2));
  fp2 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = rc::ld32(eax);
  rc::st32(edi + 0x50u, eax);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x54u, float(fp3));
  rc::stf32(edi + 0x58u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x00595936:
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x1u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595936 = a;
  lzb_595936 = b;
  }
  if ((x86::reg32(lza_595936) != x86::reg32(lzb_595936))) goto L_0x00595f54;
  ebx = x86::reg32(eax - 0x1u);
  edx = 0;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a & b);
  lzr_595944 = r;
  }
  s_048 = edx;
  s_030 = ebx;
  if ((x86::sreg32(x86::reg32(lzr_595944)) <= 0)) goto L_0x00595685;
  ecx = x86::reg32(edi + 0x4u);
  s_034 = ecx;
  goto L_0x00595960;
L_0x00595960:
  fp1 = x86::Float(rc::ldf32(edi + 0x40u));
  fp1 = psub(fp1, Float(rc::ldf32(ecx - 0x4u)));
  fp2 = x86::Float(rc::ldf32(edi + 0x44u));
  fp2 = psub(fp2, Float(rc::ldf32(ecx)));
  fp3 = x86::Float(rc::ldf32(edi + 0x48u));
  fp3 = psub(fp3, Float(rc::ldf32(ecx + 0x4u)));
  s_1e8 = bits(fp3);
  eax = s_1e8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0d8 = eax;
  s_0d0 = bits(fp2);
  eax = x86::reg32(edx + 0x1u);
  edx = s_040;
  s_0d4 = bits(fp1);
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_59599d = a;
  lzb_59599d = b;
  }
  s_064 = eax;
  s_044 = eax;
  if ((x86::sreg32(x86::reg32(lza_59599d)) >= x86::sreg32(x86::reg32(lzb_59599d)))) goto L_0x00595e4c;
  edx = x86::reg32(ecx + 0x14u);
  s_024 = edx;
  goto L_0x005959c0;
L_0x005959b6:
  edx = s_024;
  ebx = x86::reg32(ebx);
L_0x005959c0:
  fp1 = x86::Float(rc::ldf32(edx - 0x8u));
  fp1 = psub(fp1, Float(rc::ldf32(ecx - 0x4u)));
  fp2 = x86::Float(rc::ldf32(ecx));
  fp2 = psub(Float(rc::ldf32(edx - 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = psub(Float(rc::ldf32(edx)), fp3);
  s_198 = bits(fp3);
  eax = s_198;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_070 = bits(fp2);
  s_078 = eax;
  s_074 = bits(fp1);
  fp1 = x86::Float(fl(s_070));
  fp1 = pmul(fp1, Float(fl(s_0d0)));
  fp2 = x86::Float(fl(s_078));
  fp2 = pmul(fp2, Float(fl(s_0d8)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(fp2, Float(fl(s_0d4)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_070));
  fp2 = pmul(fp2, Float(fl(s_070)));
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(fp3, Float(fl(s_078)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_074));
  fp3 = pmul(fp3, Float(fl(s_074)));
  fp2 = padd(fp3, fp2);
  fp1 = pdiv(fp1, fp2);
  s_018 = bits(fp1);
  lzfa_595a3b = fp1;
  lzfb_595a3b = kZero;
  if (!(lzfa_595a3b < lzfb_595a3b)) goto L_0x00595a73;
  fp1 = x86::Float(rc::ldf32(edi + 0x40u));
  s_018 = 0x0u;
  fp1 = psub(fp1, Float(rc::ldf32(ecx - 0x4u)));
  fp2 = x86::Float(rc::ldf32(edi + 0x44u));
  fp2 = psub(fp2, Float(rc::ldf32(ecx)));
  fp3 = x86::Float(rc::ldf32(edi + 0x48u));
  fp3 = psub(fp3, Float(rc::ldf32(ecx + 0x4u)));
  s_1d8 = bits(fp3);
  ecx = s_1d8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(esi, float(fp2));
  rc::st32(esi + 0x8u, ecx);
  rc::stf32(esi + 0x4u, float(fp1));
  goto L_0x00595adc;
L_0x00595a73:
  fp1 = x86::Float(fl(s_018));
  lzfa_595a7f = fp1;
  lzfb_595a7f = kOne;
  if ((x86::reg8((!(lzfa_595a7f >= lzfb_595a7f) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_595a7f, lzfb_595a7f) ? 0x40 : 0)) != 0)) goto L_0x00595ab7;
  fp1 = x86::Float(rc::ldf32(edi + 0x40u));
  s_018 = 0x3f800000u;
  fp1 = psub(fp1, Float(rc::ldf32(edx - 0x8u)));
  fp2 = x86::Float(rc::ldf32(edi + 0x44u));
  fp2 = psub(fp2, Float(rc::ldf32(edx - 0x4u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x48u));
  fp3 = psub(fp3, Float(rc::ldf32(edx)));
  s_1a8 = bits(fp3);
  edx = s_1a8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(esi, float(fp2));
  rc::st32(esi + 0x8u, edx);
  rc::stf32(esi + 0x4u, float(fp1));
  goto L_0x00595adc;
L_0x00595ab7:
  s_mc = esi;
  fp1 = x86::Float(fl(s_070));
  fp1 = pmul(fp1, Float(fl(s_0d8)));
  fp2 = x86::Float(fl(s_078));
  fp2 = pmul(fp2, Float(fl(s_0d0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(Float(fl(s_0d0)), fp2);
  fp3 = x86::Float(fl(s_0d4));
  fp3 = pmul(Float(fl(s_070)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(fp3, Float(fl(s_0d4)));
  fp4 = x86::Float(fl(s_074));
  fp4 = pmul(Float(fl(s_0d8)), fp4);
  eax = s_mc;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_595ac5:
  s_m10 = esi;
  s_m18 = esi;
  ecx = s_m10;
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = pmul(fp1, Float(fl(s_078)));
  fp2 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp2 = pmul(fp2, Float(fl(s_070)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp2 = pmul(Float(fl(s_070)), fp2);
  fp3 = x86::Float(fl(s_074));
  fp3 = pmul(Float(rc::ldf32(ecx)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp3 = pmul(fp3, Float(fl(s_074)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(Float(fl(s_078)), fp4);
  eax = s_m18;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_595ad4:
L_0x00595adc:
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595adc = a;
  lzb_595adc = b;
  }
  fp1 = x86::Float(rc::ldf32(esi));
  fp1 = pmul(fp1, Float(rc::ldf32(edi)));
  s_01c = 0x0u;
  fp2 = x86::Float(rc::ldf32(esi + 0x8u));
  edx = 0x1u;
  fp2 = pmul(fp2, Float(rc::ldf32(edi + 0x8u)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(esi + 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(edi + 0x4u)));
  fp1 = padd(fp1, fp2);
  if ((x86::sreg32(x86::reg32(lza_595adc)) < x86::sreg32(x86::reg32(lzb_595adc)))) goto L_0x00595bd8;
  ebx = 0x4u;
  ecx = x86::reg32(edi + 0x24u);
L_0x00595b0e:
  fp2 = x86::Float(rc::ldf32(ecx - 0x10u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx - 0x14u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx - 0xcu));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_595b2b = fp2;
  lzfb_595b2b = fp1;
  if ((x86::reg8((!(lzfa_595b2b >= lzfb_595b2b) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_595b2b, lzfb_595b2b) ? 0x40 : 0)) != 0)) goto L_0x00595b3a;
  fp1 = x86::Float(fp1);
  s_01c = edx;
  fp1 = x86::Float(fl(s_014));
L_0x00595b3a:
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_595b56 = fp2;
  lzfb_595b56 = fp1;
  if ((x86::reg8((!(lzfa_595b56 >= lzfb_595b56) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_595b56, lzfb_595b56) ? 0x40 : 0)) != 0)) goto L_0x00595b68;
  eax = x86::reg32(ebx - 0x2u);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_014));
  s_01c = eax;
L_0x00595b68:
  fp2 = x86::Float(rc::ldf32(ecx + 0x10u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x14u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0xcu));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_595b85 = fp2;
  lzfb_595b85 = fp1;
  if ((x86::reg8((!(lzfa_595b85 >= lzfb_595b85) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_595b85, lzfb_595b85) ? 0x40 : 0)) != 0)) goto L_0x00595b97;
  eax = x86::reg32(ebx - 0x1u);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_014));
  s_01c = eax;
L_0x00595b97:
  fp2 = x86::Float(rc::ldf32(ecx + 0x20u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x24u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x1cu));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_595bb4 = fp2;
  lzfb_595bb4 = fp1;
  if ((x86::reg8((!(lzfa_595bb4 >= lzfb_595bb4) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_595bb4, lzfb_595bb4) ? 0x40 : 0)) != 0)) goto L_0x00595bc3;
  fp1 = x86::Float(fp1);
  s_01c = ebx;
  fp1 = x86::Float(fl(s_014));
L_0x00595bc3:
  eax = s_040;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x40u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595bd0 = a;
  lzb_595bd0 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_595bd0)) < x86::sreg32(x86::reg32(lzb_595bd0)))) goto L_0x00595b0e;
L_0x00595bd8:
  ebx = s_040;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595bdc = a;
  lzb_595bdc = b;
  }
  if ((x86::sreg32(x86::reg32(lza_595bdc)) >= x86::sreg32(x86::reg32(lzb_595bdc)))) goto L_0x00595c23;
  ecx = edx;
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  ecx = x86::reg32(ecx + edi + 0x4u);
L_0x00595bf0:
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_595c0c = fp2;
  lzfb_595c0c = fp1;
  if ((x86::reg8((!(lzfa_595c0c >= lzfb_595c0c) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_595c0c, lzfb_595c0c) ? 0x40 : 0)) != 0)) goto L_0x00595c1b;
  fp1 = x86::Float(fp1);
  s_01c = edx;
  fp1 = x86::Float(fl(s_014));
L_0x00595c1b:
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595c1f = a;
  lzb_595c1f = b;
  }
  if ((x86::sreg32(x86::reg32(lza_595c1f)) < x86::sreg32(x86::reg32(lzb_595c1f)))) goto L_0x00595bf0;
L_0x00595c23:
  fp2 = x86::Float(rc::ldf32(edi + 0x48u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x8u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x44u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(edi + 0x40u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp2 = padd(fp2, fp3);
  cmpa = fp2;
  cmpb = fp1;
  fp1 = x86::Float(fp1);
  if (!(cmpa > cmpb)) goto L_0x00595e24;
  fp1 = x86::Float(fl(s_070));
  eax = s_034;
  fp1 = pmul(fp1, Float(fl(s_018)));
  ecx = s_044;
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(Float(fl(s_018)), fp2);
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(Float(fl(s_018)), fp3);
  s_1c8 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0a0 = bits(fp2);
  s_0a4 = bits(fp1);
  fp1 = x86::Float(fl(s_1c8));
  fp2 = x86::Float(fl(s_0a0));
  fp2 = padd(fp2, Float(rc::ldf32(eax - 0x4u)));
  fp3 = x86::Float(rc::ldf32(eax));
  fp3 = padd(Float(fl(s_0a4)), fp3);
  fp4 = x86::Float(rc::ldf32(eax + 0x4u));
  fp4 = padd(fp1, fp4);
  s_058 = bits(fp4);
  edx = s_058;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x80u, float(fp3));
  rc::st32(edi + 0x88u, edx);
  edx = s_048;
  rc::stf32(edi + 0x84u, float(fp2));
  eax = rc::ld32(edi + 0x40u);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x44u));
  fp2 = x86::Float(rc::ldf32(edi + 0x48u));
  rc::st32(edi + 0x90u, eax);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  eax = s_01c;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595cd4 = a;
  lzb_595cd4 = b;
  }
  rc::stf32(edi + 0x94u, float(fp2));
  rc::stf32(edi + 0x98u, float(fp1));
  fp1 = x86::Float(rc::ldf32(edi + 0x90u));
  fp1 = psub(fp1, Float(rc::ldf32(edi + 0x80u)));
  fp2 = x86::Float(rc::ldf32(edi + 0x94u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x84u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x98u));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x88u)));
  fp4 = fp3;
  fp4 = pmul(fp4, fp3);
  fp5 = fp2;
  fp5 = pmul(fp5, fp2);
  fp4 = padd(fp5, fp4);
  fp5 = fp1;
  fp5 = pmul(fp5, fp1);
  fp4 = padd(fp5, fp4);
  fp1 = x86::Float(fp4);
  fp3 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  if ((x86::reg32(lza_595cd4) == x86::reg32(lzb_595cd4))) goto L_0x00595e66;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595d22 = a;
  lzb_595d22 = b;
  }
  if ((x86::reg32(lza_595d22) == x86::reg32(lzb_595d22))) goto L_0x00595e66;
  lzfa_595d30 = fp1;
  lzfb_595d30 = x86::Float(fl(s_068));
  if (!(lzfa_595d30 < lzfb_595d30)) goto L_0x00595e20;
  fp2 = x86::Float(rc::ldf32(edi + 0x84u));
  eax = rc::ld32(esi);
  s_164 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x88u));
  s_140 = eax;
  eax = rc::ld32(esi + 0x4u);
  s_168 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x94u));
  s_144 = eax;
  eax = rc::ld32(esi + 0x8u);
  s_154 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x98u));
  s_148 = eax;
  eax = rc::ld32(edi + 0x80u);
  s_158 = bits(fp2);
  s_160 = eax;
  eax = rc::ld32(edi + 0x90u);
  s_068 = bits(fp1);
  fp1 = x86::Float(fl(s_018));
  s_150 = eax;
  cmpa = fp1;
  cmpb = kZero;
  ebx = 0x1u;
  s_0cc = ebx;
  if (!(cmpa == cmpb)) goto L_0x00595dce;
  s_13c = ebx;
  goto L_0x00595e01;
L_0x00595dce:
  fp1 = x86::Float(fl(s_018));
  lzfa_595dda = fp1;
  lzfb_595dda = kOne;
  if (!(lzfa_595dda == lzfb_595dda)) goto L_0x00595def;
  s_13c = ebx;
  IA[0] = ecx;
  goto L_0x00595e08;
L_0x00595def:
  s_13c = 0x2u;
  IA[1] = ecx;
L_0x00595e01:
  IA[0] = edx;
L_0x00595e08:
  s_128 = ebx;
  ebx = s_040;
  IB[0] = 0x0u;
  goto L_0x00595e28;
L_0x00595e20:
  fp1 = x86::Float(fp1);
  goto L_0x00595e28;
L_0x00595e24:
  ecx = s_044;
L_0x00595e28:
  edx = s_024;
  {
  const x86::reg32 r = x86::reg32(ecx + 1);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595e30 = a;
  lzb_595e30 = b;
  }
  ebx = s_030;
  s_044 = ecx;
  ecx = s_034;
  s_024 = edx;
  if ((x86::sreg32(x86::reg32(lza_595e30)) < x86::sreg32(x86::reg32(lzb_595e30)))) goto L_0x005959b6;
  eax = s_064;
L_0x00595e4c:
  edx = eax;
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595e51 = a;
  lzb_595e51 = b;
  }
  s_048 = edx;
  s_034 = ecx;
  if ((x86::sreg32(x86::reg32(lza_595e51)) < x86::sreg32(x86::reg32(lzb_595e51)))) goto L_0x00595960;
  app->allowPreemption(cpu);
  goto L_0x00595681;
L_0x00595e66:
  fp2 = x86::Float(fl(s_018));
  lzfa_595e72 = fp2;
  lzfb_595e72 = kZero;
  if (!(lzfa_595e72 == lzfb_595e72)) goto L_0x00595eae;
  rc::st32(edi + 0xc0u, 0x1u);
  ecx = rc::ld32(edi + edx * 4 + 0xb0u);
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  rc::st32(edi + 0xb0u, ecx);
  fp2 = x86::Float(rc::ldf32(edx + 0x4u));
  fp3 = x86::Float(rc::ldf32(edx + 0x8u));
  edx = rc::ld32(edx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi, edx);
  rc::stf32(edi + 0x4u, float(fp3));
  rc::stf32(edi + 0x8u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x00595eae:
  fp2 = x86::Float(fl(s_018));
  lzfa_595eba = fp2;
  lzfb_595eba = kOne;
  if (!(lzfa_595eba == lzfb_595eba)) goto L_0x00595ef6;
  rc::st32(edi + 0xc0u, 0x1u);
  eax = rc::ld32(edi + ecx * 4 + 0xb0u);
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  rc::st32(edi + 0xb0u, eax);
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  ecx = rc::ld32(ecx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi, ecx);
  rc::stf32(edi + 0x4u, float(fp3));
  rc::stf32(edi + 0x8u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x00595ef6:
  rc::st32(edi + 0xc0u, 0x2u);
  eax = rc::ld32(edi + edx * 4 + 0xb0u);
  rc::st32(edi + 0xb0u, eax);
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  fp2 = x86::Float(rc::ldf32(edx + edi + 0x4u));
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  fp3 = x86::Float(rc::ldf32(edx + 0x8u));
  edx = rc::ld32(edx);
  rc::st32(edi, edx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x4u, float(fp3));
  rc::stf32(edi + 0x8u, float(fp2));
  eax = rc::ld32(edi + ecx * 4 + 0xb0u);
  rc::st32(edi + 0xb4u, eax);
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  fp2 = x86::Float(rc::ldf32(ecx + edi + 0x4u));
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  ecx = rc::ld32(ecx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x10u, ecx);
  rc::stf32(edi + 0x14u, float(fp3));
  rc::stf32(edi + 0x18u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x00595f54:
  ecx = x86::reg32(eax - 0x1u);
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = edx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595f57 = a;
  lzb_595f57 = b;
  }
  s_048 = edx;
  s_030 = ecx;
  if ((x86::sreg32(x86::reg32(lza_595f57)) <= x86::sreg32(x86::reg32(lzb_595f57)))) goto L_0x00595685;
L_0x00595f67:
  eax = s_048;
  ecx = s_040;
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_595f70 = a;
  lzb_595f70 = b;
  }
  s_064 = eax;
  s_044 = eax;
  if ((x86::sreg32(x86::reg32(lza_595f70)) >= x86::sreg32(x86::reg32(lzb_595f70)))) goto L_0x00596dfd;
  edx = s_048;
  eax = s_04c;
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  s_02c = edx;
  s_034 = eax;
L_0x00595f96:
  ecx = s_044;
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  fp1 = x86::Float(rc::ldf32(ecx + edi));
  eax = x86::reg32(ecx + edi);
  ecx = s_02c;
  fp1 = psub(fp1, Float(rc::ldf32(ecx)));
  s_08c = eax;
  fp2 = x86::Float(rc::ldf32(eax + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(ecx + 0x4u)));
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = s_034;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a & b);
  lzr_595fbd = r;
  }
  fp3 = psub(fp3, Float(rc::ldf32(ecx + 0x8u)));
  s_1e8 = bits(fp3);
  edx = s_1e8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_070 = bits(fp2);
  s_078 = edx;
  s_074 = bits(fp1);
  s_03c = 0x0u;
  fp1 = x86::Float(fl(s_070));
  fp1 = pmul(fp1, Float(fl(s_070)));
  fp2 = x86::Float(fl(s_078));
  fp2 = pmul(fp2, Float(fl(s_078)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(fp2, Float(fl(s_074)));
  fp1 = padd(fp1, fp2);
  s_17c = bits(fp1);
  if ((x86::sreg32(x86::reg32(lzr_595fbd)) <= 0)) goto L_0x00596de8;
L_0x00596010:
  edx = s_03c;
  ecx = s_02c;
  eax = x86::reg32(edx + 0x4u);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp1 = x86::Float(rc::ldf32(eax + edi));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  fp1 = psub(fp1, Float(rc::ldf32(ecx)));
  s_024 = eax;
  fp2 = x86::Float(rc::ldf32(eax + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(ecx + 0x4u)));
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = x86::reg32(edx + 0x1u);
  fp3 = psub(fp3, Float(rc::ldf32(ecx + 0x8u)));
  s_2b8 = bits(fp3);
  ecx = s_2b8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0d0 = bits(fp2);
  s_0d8 = ecx;
  ecx = s_04c;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_59605a = a;
  lzb_59605a = b;
  }
  s_0d4 = bits(fp1);
  fp1 = x86::Float(fl(s_070));
  fp1 = pmul(fp1, Float(fl(s_0d0)));
  s_06c = eax;
  fp2 = x86::Float(fl(s_078));
  s_038 = eax;
  fp2 = pmul(fp2, Float(fl(s_0d8)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(fp2, Float(fl(s_0d4)));
  fp1 = padd(fp1, fp2);
  s_088 = bits(fp1);
  if ((x86::sreg32(x86::reg32(lza_59605a)) >= x86::sreg32(x86::reg32(lzb_59605a)))) goto L_0x00596dd6;
  ecx = x86::reg32(ecx);
L_0x005960a0:
  edx = s_038;
  eax = s_024;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  fp1 = x86::Float(rc::ldf32(edx + edi));
  ebx = x86::reg32(edx + edi);
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax + 0x8u)));
  s_2a8 = bits(fp3);
  eax = s_2a8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_090 = bits(fp2);
  s_098 = eax;
  s_094 = bits(fp1);
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_090)));
  fp2 = x86::Float(fl(s_098));
  fp2 = pmul(fp2, Float(fl(s_098)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(fp2, Float(fl(s_094)));
  fp1 = padd(fp2, fp1);
  fp2 = x86::Float(fl(s_070));
  fp2 = pmul(Float(fl(s_090)), fp2);
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(Float(fl(s_098)), fp3);
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_074));
  fp3 = pmul(Float(fl(s_094)), fp3);
  fp2 = padd(fp2, fp3);
  s_01c = bits(fp2);
  fp2 = x86::Float(fl(s_090));
  fp2 = pmul(fp2, Float(fl(s_0d0)));
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(fp3, Float(fl(s_0d8)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_094));
  fp3 = pmul(fp3, Float(fl(s_0d4)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_17c));
  fp3 = pmul(fp1, fp3);
  fp4 = x86::Float(fl(s_01c));
  fp4 = pmul(fp4, Float(fl(s_01c)));
  fp3 = psub(fp3, fp4);
  fp4 = x86::Float(fl(s_088));
  fp4 = pmul(fp1, fp4);
  fp5 = x86::Float(fl(s_01c));
  fp5 = pmul(fp5, fp2);
  fp4 = psub(fp4, fp5);
  fp5 = fp3;
  fp5 = cpu.fpu.abs(fp5);
  lzfa_59619c = fp5;
  lzfb_59619c = kEps;
  if ((x86::reg8((!(lzfa_59619c >= lzfb_59619c) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_59619c, lzfb_59619c) ? 0x40 : 0)) != 0)) goto L_0x005961c4;
  fp4 = pdiv(fp4, fp3);
  s_018 = bits(fp4);
  fp3 = x86::Float(fp3);
  fp3 = x86::Float(fl(s_018));
  lzfa_5961b5 = fp3;
  lzfb_5961b5 = kZero;
  if (!(lzfa_5961b5 < lzfb_5961b5)) goto L_0x005961df;
  s_018 = 0x0u;
  goto L_0x005961f8;
L_0x005961c4:
  fp4 = pmul(fp3, fp4);
  cmpa = fp4;
  cmpb = kZero;
  fp3 = x86::Float(fp3);
  if ((cmpa > cmpb)) goto L_0x005961f0;
  s_018 = 0x0u;
  goto L_0x005961f8;
L_0x005961df:
  fp3 = x86::Float(fl(s_018));
  lzfa_5961eb = fp3;
  lzfb_5961eb = kOne;
  if ((x86::reg8((!(lzfa_5961eb >= lzfb_5961eb) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_5961eb, lzfb_5961eb) ? 0x40 : 0)) != 0)) goto L_0x005961f8;
L_0x005961f0:
  s_018 = 0x3f800000u;
L_0x005961f8:
  fp3 = x86::Float(fl(s_01c));
  fp3 = pmul(fp3, Float(fl(s_018)));
  fp3 = psub(fp3, fp2);
  fp3 = pdiv(fp3, fp1);
  s_020 = bits(fp3);
  fp2 = x86::Float(fp2);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_020));
  lzfa_596218 = fp1;
  lzfb_596218 = kZero;
  if ((x86::reg8((!(lzfa_596218 >= lzfb_596218) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_596218, lzfb_596218) ? 0x40 : 0)) != 0)) goto L_0x00596716;
  fp1 = x86::Float(fl(s_020));
  lzfa_59622d = fp1;
  lzfb_59622d = kOne;
  if (!(lzfa_59622d < lzfb_59622d)) goto L_0x00596716;
  fp1 = x86::Float(fl(s_018));
  lzfa_596242 = fp1;
  lzfb_596242 = kZero;
  if ((x86::reg8((!(lzfa_596242 >= lzfb_596242) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_596242, lzfb_596242) ? 0x40 : 0)) != 0)) goto L_0x0059664e;
  fp1 = x86::Float(fl(s_018));
  lzfa_596257 = fp1;
  lzfb_596257 = kOne;
  if (!(lzfa_596257 < lzfb_596257)) goto L_0x0059664e;
  s_mc = esi;
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_078)));
  fp2 = x86::Float(fl(s_098));
  fp2 = pmul(fp2, Float(fl(s_070)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(Float(fl(s_070)), fp2);
  fp3 = x86::Float(fl(s_074));
  fp3 = pmul(Float(fl(s_090)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(fp3, Float(fl(s_074)));
  fp4 = x86::Float(fl(s_094));
  fp4 = pmul(Float(fl(s_078)), fp4);
  eax = s_mc;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_59626e:
  s_m10 = esi;
  s_m14 = esi;
  ecx = s_m10;
  fp1 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = x86::Float(rc::ldf32(ecx));
  fp4 = fp3;
  fp4 = pmul(fp4, fp3);
  fp5 = fp2;
  fp5 = pmul(fp5, fp2);
  fp4 = padd(fp5, fp4);
  fp5 = fp1;
  fp5 = pmul(fp5, fp1);
  fp4 = padd(fp5, fp4);
  fp4 = cpu.fpu.sqrt(fp4);
  fp1 = x86::Float(fp4);
  fp3 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  lzfa_43cf3c = fp1;
  lzfb_43cf3c = kZero;
  if ((lzfa_43cf3c == lzfb_43cf3c)) goto L_0x0043cf6c_596275;
  fp1 = pdiv(kOne, fp1);
  eax = rc::ld32(ecx + 0x8u);
  s_m10 = eax;
  eax = s_m14;
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = fp1;
  fp3 = pmul(fp3, Float(rc::ldf32(ecx)));
  rc::stf32(eax, float(fp3));
  fp2 = pmul(fp1, fp2);
  rc::stf32(eax + 0x4u, float(fp2));
  fp2 = x86::Float(fl(s_m10));
  fp2 = pmul(fp1, fp2);
  rc::stf32(eax + 0x8u, float(fp2));
  fp1 = x86::Float(fp1);
  goto L_ret_596275;
L_0x0043cf6c_596275:
  eax = s_m14;
  fp1 = x86::Float(fp1);
  ecx = 0;
  rc::st32(eax, 0x3f800000u);
  rc::st32(eax + 0x4u, ecx);
  rc::st32(eax + 0x8u, ecx);
L_ret_596275:
  fp1 = x86::Float(fl(s_0d4));
  fp1 = pmul(fp1, Float(rc::ldf32(esi + 0x4u)));
  fp2 = x86::Float(fl(s_0d0));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_0d8));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x8u)));
  fp1 = padd(fp1, fp2);
  lzfa_5962a6 = fp1;
  lzfb_5962a6 = kZero;
  if (!(lzfa_5962a6 < lzfb_5962a6)) goto L_0x005962d1;
  fp1 = x86::Float(rc::ldf32(esi));
  fp1 = pneg(fp1);
  fp2 = x86::Float(rc::ldf32(esi + 0x4u));
  fp2 = pneg(fp2);
  fp3 = x86::Float(rc::ldf32(esi + 0x8u));
  fp3 = pneg(fp3);
  s_258 = bits(fp3);
  eax = s_258;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::st32(esi + 0x8u, eax);
  rc::stf32(esi, float(fp2));
  rc::stf32(esi + 0x4u, float(fp1));
L_0x005962d1:
  fp1 = x86::Float(fl(s_070));
  eax = s_02c;
  fp1 = pmul(fp1, Float(fl(s_018)));
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(Float(fl(s_018)), fp2);
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(Float(fl(s_018)), fp3);
  s_218 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_290 = bits(fp2);
  fp2 = x86::Float(fl(s_218));
  fp3 = x86::Float(fl(s_290));
  fp3 = padd(fp3, Float(rc::ldf32(eax)));
  s_1c0 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = padd(fp1, fp3);
  s_1c4 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = s_024;
  fp3 = padd(fp2, fp3);
  fp1 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(rc::ldf32(eax));
  fp2 = psub(fp2, Float(fl(s_1c0)));
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = psub(fp3, Float(fl(s_1c4)));
  fp4 = x86::Float(rc::ldf32(eax + 0x8u));
  fp4 = psub(fp4, fp1);
  s_238 = bits(fp4);
  ecx = s_238;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  s_0e0 = bits(fp3);
  s_0e8 = ecx;
  s_0e4 = bits(fp2);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_0e8)));
  fp2 = x86::Float(fl(s_098));
  fp2 = pmul(fp2, Float(fl(s_0e0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(Float(fl(s_0e0)), fp2);
  fp3 = x86::Float(fl(s_0e4));
  fp3 = pmul(Float(fl(s_090)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(fp3, Float(fl(s_0e4)));
  fp4 = x86::Float(fl(s_094));
  fp4 = pmul(Float(fl(s_0e8)), fp4);
  fp3 = psub(fp3, fp4);
  s_050 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_054 = bits(fp2);
  s_058 = bits(fp1);
L_ret_596381:
  fp1 = x86::Float(fl(s_050));
  fp1 = pmul(fp1, Float(fl(s_098)));
  fp2 = x86::Float(fl(s_058));
  fp2 = pmul(fp2, Float(fl(s_090)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_054));
  fp2 = pmul(Float(fl(s_090)), fp2);
  fp3 = x86::Float(fl(s_094));
  fp3 = pmul(Float(fl(s_050)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_058));
  fp3 = pmul(fp3, Float(fl(s_094)));
  fp4 = x86::Float(fl(s_054));
  fp4 = pmul(Float(fl(s_098)), fp4);
  fp3 = psub(fp3, fp4);
  s_050 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_054 = bits(fp2);
  s_058 = bits(fp1);
L_ret_596398:
  edx = s_020;
  ecx = s_024;
  s_m1c = edx;
  s_m24 = ecx;
  fp1 = x86::Float(fl(s_m1c));
  fp1 = pmul(fp1, Float(fl(s_094)));
  ecx = s_m24;
  fp1 = padd(fp1, Float(rc::ldf32(ecx + 0x4u)));
  fp2 = x86::Float(fl(s_m1c));
  fp2 = pmul(fp2, Float(fl(s_098)));
  fp2 = padd(fp2, Float(rc::ldf32(ecx + 0x8u)));
  fp3 = x86::Float(fl(s_m1c));
  fp3 = pmul(fp3, Float(fl(s_090)));
  fp3 = padd(fp3, Float(rc::ldf32(ecx)));
  s_180 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_184 = bits(fp2);
  s_188 = bits(fp1);
L_ret_5963b7:
  fp1 = x86::Float(fl(s_184));
  fp2 = x86::Float(fl(s_188));
  fp3 = x86::Float(fl(s_180));
  eax = s_02c;
  fp3 = psub(fp3, Float(rc::ldf32(eax)));
  s_0e0 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = psub(fp1, fp3);
  s_0e4 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  fp3 = psub(fp2, fp3);
  s_0e8 = bits(fp3);
  fp2 = x86::Float(fp2);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_070));
  fp1 = pmul(fp1, Float(fl(s_0e8)));
  fp2 = x86::Float(fl(s_078));
  fp2 = pmul(fp2, Float(fl(s_0e0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(Float(fl(s_0e0)), fp2);
  fp3 = x86::Float(fl(s_0e4));
  fp3 = pmul(Float(fl(s_070)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(fp3, Float(fl(s_0e4)));
  fp4 = x86::Float(fl(s_074));
  fp4 = pmul(Float(fl(s_0e8)), fp4);
  fp3 = psub(fp3, fp4);
  s_0a0 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0a4 = bits(fp2);
  s_0a8 = bits(fp1);
L_ret_596405:
  fp1 = x86::Float(fl(s_0a0));
  fp1 = pmul(fp1, Float(fl(s_078)));
  fp2 = x86::Float(fl(s_0a8));
  fp2 = pmul(fp2, Float(fl(s_070)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_0a4));
  fp2 = pmul(Float(fl(s_070)), fp2);
  fp3 = x86::Float(fl(s_074));
  fp3 = pmul(Float(fl(s_0a0)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_0a8));
  fp3 = pmul(fp3, Float(fl(s_074)));
  fp4 = x86::Float(fl(s_0a4));
  fp4 = pmul(Float(fl(s_078)), fp4);
  fp3 = psub(fp3, fp4);
  s_0a0 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0a4 = bits(fp2);
  s_0a8 = bits(fp1);
L_ret_596422:
  eax = s_08c;
  fp1 = x86::Float(rc::ldf32(eax + 0x8u));
  fp1 = pmul(fp1, Float(rc::ldf32(esi + 0x8u)));
  fp2 = x86::Float(rc::ldf32(eax + 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x4u)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(esi));
  fp2 = pmul(fp2, Float(rc::ldf32(eax)));
  eax = s_02c;
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(eax + 0x8u));
  fp2 = pmul(Float(rc::ldf32(esi + 0x8u)), fp2);
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = pmul(Float(rc::ldf32(esi + 0x4u)), fp3);
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(eax));
  fp3 = pmul(Float(rc::ldf32(esi)), fp3);
  fp2 = padd(fp2, fp3);
  s_01c = bits(fp2);
  lzfa_596465 = fp2;
  lzfb_596465 = fp1;
  if (!(lzfa_596465 < lzfb_596465)) goto L_0x00596470;
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_01c));
L_0x00596470:
  fp2 = x86::Float(rc::ldf32(ebx + 0x8u));
  ecx = s_024;
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x8u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(esi));
  fp3 = pmul(fp3, Float(rc::ldf32(ebx)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp3 = pmul(Float(rc::ldf32(esi + 0x8u)), fp3);
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(Float(rc::ldf32(esi + 0x4u)), fp4);
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx));
  fp4 = pmul(Float(rc::ldf32(esi)), fp4);
  fp3 = padd(fp3, fp4);
  s_01c = bits(fp3);
  lzfa_5964a4 = fp3;
  lzfb_5964a4 = fp2;
  if (!(lzfa_5964a4 < lzfb_5964a4)) goto L_0x005964af;
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_01c));
L_0x005964af:
  edx = s_08c;
  fp1 = psub(fp2, fp1);
  fp2 = x86::Float(fl(s_054));
  eax = s_02c;
  fp2 = pmul(fp2, Float(rc::ldf32(edx + 0x4u)));
  fp3 = x86::Float(fl(s_058));
  fp3 = pmul(fp3, Float(rc::ldf32(edx + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_050));
  fp3 = pmul(fp3, Float(rc::ldf32(edx)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_054));
  fp3 = pmul(fp3, Float(rc::ldf32(eax + 0x4u)));
  fp4 = x86::Float(fl(s_058));
  fp4 = pmul(fp4, Float(rc::ldf32(eax + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(fl(s_050));
  fp4 = pmul(fp4, Float(rc::ldf32(eax)));
  fp3 = padd(fp3, fp4);
  s_01c = bits(fp3);
  lzfa_5964f4 = fp3;
  lzfb_5964f4 = fp2;
  if (!(lzfa_5964f4 < lzfb_5964f4)) goto L_0x005964ff;
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_01c));
L_0x005964ff:
  fp3 = x86::Float(fl(s_058));
  fp3 = pmul(fp3, Float(rc::ldf32(ebx + 0x8u)));
  fp4 = x86::Float(fl(s_054));
  fp4 = pmul(fp4, Float(rc::ldf32(ebx + 0x4u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(fl(s_050));
  fp4 = pmul(fp4, Float(rc::ldf32(ebx)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(fl(s_054));
  fp4 = pmul(fp4, Float(rc::ldf32(ecx + 0x4u)));
  fp5 = x86::Float(fl(s_058));
  fp5 = pmul(fp5, Float(rc::ldf32(ecx + 0x8u)));
  fp4 = padd(fp4, fp5);
  fp5 = x86::Float(fl(s_050));
  fp5 = pmul(fp5, Float(rc::ldf32(ecx)));
  fp4 = padd(fp4, fp5);
  s_01c = bits(fp4);
  lzfa_596537 = fp4;
  lzfb_596537 = fp3;
  if (!(lzfa_596537 < lzfb_596537)) goto L_0x00596542;
  fp3 = x86::Float(fp3);
  fp3 = x86::Float(fl(s_01c));
L_0x00596542:
  fp2 = psub(fp3, fp2);
  s_01c = bits(fp2);
  lzfa_59654c = fp2;
  lzfb_59654c = fp1;
  if ((x86::reg8((!(lzfa_59654c >= lzfb_59654c) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_59654c, lzfb_59654c) ? 0x40 : 0)) != 0)) goto L_0x0059656b;
  eax = s_050;
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_01c));
  rc::st32(esi, eax);
  eax = s_054;
  rc::st32(esi + 0x4u, eax);
  eax = s_058;
  rc::st32(esi + 0x8u, eax);
L_0x0059656b:
  fp2 = x86::Float(fl(s_0a4));
  eax = s_02c;
  fp2 = pmul(fp2, Float(rc::ldf32(edx + 0x4u)));
  fp3 = x86::Float(fl(s_0a8));
  fp3 = pmul(fp3, Float(rc::ldf32(edx + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_0a0));
  fp3 = pmul(fp3, Float(rc::ldf32(edx)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(fl(s_0a4));
  fp3 = pmul(fp3, Float(rc::ldf32(eax + 0x4u)));
  fp4 = x86::Float(fl(s_0a8));
  fp4 = pmul(fp4, Float(rc::ldf32(eax + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(fl(s_0a0));
  fp4 = pmul(fp4, Float(rc::ldf32(eax)));
  fp3 = padd(fp3, fp4);
  s_01c = bits(fp3);
  lzfa_5965b9 = fp3;
  lzfb_5965b9 = fp2;
  if (!(lzfa_5965b9 < lzfb_5965b9)) goto L_0x005965c4;
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_01c));
L_0x005965c4:
  fp3 = x86::Float(fl(s_0a4));
  fp3 = pmul(fp3, Float(rc::ldf32(ebx + 0x4u)));
  fp4 = x86::Float(fl(s_0a8));
  fp4 = pmul(fp4, Float(rc::ldf32(ebx + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(fl(s_0a0));
  fp4 = pmul(fp4, Float(rc::ldf32(ebx)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(fl(s_0a4));
  fp4 = pmul(fp4, Float(rc::ldf32(ecx + 0x4u)));
  fp5 = x86::Float(fl(s_0a8));
  fp5 = pmul(fp5, Float(rc::ldf32(ecx + 0x8u)));
  fp4 = padd(fp4, fp5);
  fp5 = x86::Float(fl(s_0a0));
  fp5 = pmul(fp5, Float(rc::ldf32(ecx)));
  fp4 = padd(fp4, fp5);
  s_01c = bits(fp4);
  lzfa_59660e = fp4;
  lzfb_59660e = fp3;
  if (!(lzfa_59660e < lzfb_59660e)) goto L_0x00596619;
  fp3 = x86::Float(fp3);
  fp3 = x86::Float(fl(s_01c));
L_0x00596619:
  fp3 = psub(fp3, fp2);
  cmpa = fp3;
  cmpb = fp1;
  fp2 = x86::Float(fp2);
  fp1 = x86::Float(fp1);
  if (!(cmpa > cmpb)) goto L_0x0059687d;
  ecx = s_0a0;
  edx = s_0a4;
  eax = s_0a8;
  rc::st32(esi, ecx);
  rc::st32(esi + 0x4u, edx);
  rc::st32(esi + 0x8u, eax);
  goto L_0x0059687d;
L_0x0059664e:
  fp1 = x86::Float(fl(s_070));
  eax = s_02c;
  fp1 = pmul(fp1, Float(fl(s_018)));
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(fp2, Float(fl(s_018)));
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(Float(fl(s_018)), fp3);
  s_278 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_1f0 = bits(fp2);
  fp2 = x86::Float(fl(s_278));
  fp3 = x86::Float(fl(s_1f0));
  fp3 = padd(fp3, Float(rc::ldf32(eax)));
  s_1a0 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = padd(fp1, fp3);
  s_1a4 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = s_024;
  fp3 = padd(fp2, fp3);
  fp1 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(rc::ldf32(eax));
  fp2 = psub(fp2, Float(fl(s_1a0)));
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = psub(fp3, Float(fl(s_1a4)));
  fp4 = x86::Float(rc::ldf32(eax + 0x8u));
  fp4 = psub(fp4, fp1);
  s_mc = esi;
  s_208 = bits(fp4);
  ecx = s_208;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  s_0e0 = bits(fp3);
  s_0e4 = bits(fp2);
  s_0e8 = ecx;
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_0e8)));
  fp2 = x86::Float(fl(s_098));
  fp2 = pmul(fp2, Float(fl(s_0e0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(Float(fl(s_0e0)), fp2);
  fp3 = x86::Float(fl(s_0e4));
  fp3 = pmul(Float(fl(s_090)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(fp3, Float(fl(s_0e4)));
  fp4 = x86::Float(fl(s_094));
  fp4 = pmul(Float(fl(s_0e8)), fp4);
  eax = s_mc;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_5966fa:
  s_m10 = esi;
  s_m18 = esi;
  ecx = s_m10;
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = pmul(fp1, Float(fl(s_098)));
  fp2 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp2 = pmul(fp2, Float(fl(s_090)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp2 = pmul(Float(fl(s_090)), fp2);
  fp3 = x86::Float(fl(s_094));
  fp3 = pmul(Float(rc::ldf32(ecx)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp3 = pmul(fp3, Float(fl(s_094)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(Float(fl(s_098)), fp4);
  eax = s_m18;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_596709:
  goto L_0x0059687d;
L_0x00596716:
  fp1 = x86::Float(fl(s_020));
  s_020 = 0x0u;
  lzfa_59672a = fp1;
  lzfb_59672a = kZero;
  if ((lzfa_59672a <= lzfb_59672a)) goto L_0x00596737;
  s_020 = 0x3f800000u;
L_0x00596737:
  fp1 = x86::Float(fl(s_01c));
  fp1 = pmul(fp1, Float(fl(s_020)));
  fp1 = padd(fp1, Float(fl(s_088)));
  fp1 = pdiv(fp1, Float(fl(s_17c)));
  s_018 = bits(fp1);
  lzfa_596759 = fp1;
  lzfb_596759 = kZero;
  if ((x86::reg8((!(lzfa_596759 >= lzfb_596759) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_596759, lzfb_596759) ? 0x40 : 0)) != 0)) goto L_0x005967f0;
  fp1 = x86::Float(fl(s_018));
  lzfa_59676e = fp1;
  lzfb_59676e = kOne;
  if (!(lzfa_59676e < lzfb_59676e)) goto L_0x005967f0;
  edx = s_020;
  ecx = s_024;
  s_m4 = edx;
  s_mc = ecx;
  fp1 = x86::Float(fl(s_m4));
  fp1 = pmul(fp1, Float(fl(s_094)));
  ecx = s_mc;
  fp1 = padd(fp1, Float(rc::ldf32(ecx + 0x4u)));
  fp2 = x86::Float(fl(s_m4));
  fp2 = pmul(fp2, Float(fl(s_098)));
  fp2 = padd(fp2, Float(rc::ldf32(ecx + 0x8u)));
  fp3 = x86::Float(fl(s_m4));
  fp3 = pmul(fp3, Float(fl(s_090)));
  fp3 = padd(fp3, Float(rc::ldf32(ecx)));
  s_180 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_184 = bits(fp2);
  s_188 = bits(fp1);
L_ret_59678d:
  fp1 = x86::Float(fl(s_184));
  fp2 = x86::Float(fl(s_188));
  fp3 = x86::Float(fl(s_180));
  eax = s_02c;
  fp3 = psub(fp3, Float(rc::ldf32(eax)));
  s_0e0 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = psub(fp1, fp3);
  s_0e4 = bits(fp3);
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  fp3 = psub(fp2, fp3);
  s_0e8 = bits(fp3);
  s_m1c = esi;
  fp2 = x86::Float(fp2);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_070));
  fp1 = pmul(fp1, Float(fl(s_0e8)));
  fp2 = x86::Float(fl(s_078));
  fp2 = pmul(fp2, Float(fl(s_0e0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(Float(fl(s_0e0)), fp2);
  fp3 = x86::Float(fl(s_0e4));
  fp3 = pmul(Float(fl(s_070)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(fp3, Float(fl(s_0e4)));
  fp4 = x86::Float(fl(s_074));
  fp4 = pmul(Float(fl(s_0e8)), fp4);
  eax = s_m1c;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_5967d4:
  s_m20 = esi;
  s_m28 = esi;
  ecx = s_m20;
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = pmul(fp1, Float(fl(s_078)));
  fp2 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp2 = pmul(fp2, Float(fl(s_070)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp2 = pmul(Float(fl(s_070)), fp2);
  fp3 = x86::Float(fl(s_074));
  fp3 = pmul(Float(rc::ldf32(ecx)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp3 = pmul(fp3, Float(fl(s_074)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(Float(fl(s_078)), fp4);
  eax = s_m28;
  fp3 = psub(fp3, fp4);
  rc::stf32(eax, float(fp3));
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(eax + 0x4u, float(fp2));
  rc::stf32(eax + 0x8u, float(fp1));
L_ret_5967e3:
  goto L_0x0059687d;
L_0x005967f0:
  fp1 = x86::Float(fl(s_018));
  s_018 = 0x0u;
  lzfa_596804 = fp1;
  lzfb_596804 = kZero;
  if ((lzfa_596804 <= lzfb_596804)) goto L_0x00596811;
  s_018 = 0x3f800000u;
L_0x00596811:
  eax = s_018;
  edx = s_02c;
  s_m4 = eax;
  s_mc = edx;
  fp1 = x86::Float(fl(s_m4));
  fp1 = pmul(fp1, Float(fl(s_074)));
  ecx = s_mc;
  fp1 = padd(fp1, Float(rc::ldf32(ecx + 0x4u)));
  fp2 = x86::Float(fl(s_m4));
  fp2 = pmul(fp2, Float(fl(s_078)));
  fp2 = padd(fp2, Float(rc::ldf32(ecx + 0x8u)));
  fp3 = x86::Float(fl(s_m4));
  fp3 = pmul(fp3, Float(fl(s_070)));
  fp3 = padd(fp3, Float(rc::ldf32(ecx)));
  s_180 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_184 = bits(fp2);
  s_188 = bits(fp1);
L_ret_596828:
  ecx = s_020;
  s_m14 = ecx;
  eax = s_024;
  s_m1c = eax;
  fp1 = x86::Float(fl(s_m14));
  fp1 = pmul(fp1, Float(fl(s_094)));
  ecx = s_m1c;
  fp1 = padd(fp1, Float(rc::ldf32(ecx + 0x4u)));
  fp2 = x86::Float(fl(s_m14));
  fp2 = pmul(fp2, Float(fl(s_098)));
  fp2 = padd(fp2, Float(rc::ldf32(ecx + 0x8u)));
  fp3 = x86::Float(fl(s_m14));
  fp3 = pmul(fp3, Float(fl(s_090)));
  fp3 = padd(fp3, Float(rc::ldf32(ecx)));
  s_0e0 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0e4 = bits(fp2);
  s_0e8 = bits(fp1);
L_ret_596849:
  fp1 = x86::Float(fl(s_0e4));
  fp2 = x86::Float(fl(s_0e8));
  edx = s_188;
  fp3 = x86::Float(fl(s_184));
  s_1bc = edx;
  fp4 = x86::Float(fl(s_0e0));
  fp4 = psub(fp4, Float(fl(s_180)));
  rc::stf32(esi, float(fp4));
  {
  const x86::Float t = fp3;
  fp3 = fp1;
  fp1 = t;
  }
  fp3 = psub(fp3, fp1);
  rc::stf32(esi + 0x4u, float(fp3));
  fp1 = x86::Float(fp2);
  fp1 = psub(fp1, Float(fl(s_1bc)));
  rc::stf32(esi + 0x8u, float(fp1));
L_0x0059687d:
  fp1 = x86::Float(rc::ldf32(esi));
  eax = s_030;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596883 = a;
  lzb_596883 = b;
  }
  fp1 = pmul(fp1, Float(rc::ldf32(edi)));
  fp2 = x86::Float(rc::ldf32(esi + 0x8u));
  s_01c = 0x0u;
  fp2 = pmul(fp2, Float(rc::ldf32(edi + 0x8u)));
  edx = 0x1u;
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(esi + 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(edi + 0x4u)));
  fp1 = padd(fp1, fp2);
  if ((x86::sreg32(x86::reg32(lza_596883)) < x86::sreg32(x86::reg32(lzb_596883)))) goto L_0x0059697d;
  ebx = 0x4u;
  ecx = x86::reg32(edi + 0x24u);
L_0x005968b3:
  fp2 = x86::Float(rc::ldf32(ecx - 0x14u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(ecx - 0xcu));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx - 0x10u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_5968d0 = fp2;
  lzfb_5968d0 = fp1;
  if ((x86::reg8((!(lzfa_5968d0 >= lzfb_5968d0) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_5968d0, lzfb_5968d0) ? 0x40 : 0)) != 0)) goto L_0x005968df;
  fp1 = x86::Float(fp1);
  s_01c = edx;
  fp1 = x86::Float(fl(s_014));
L_0x005968df:
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(esi + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(ecx)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_5968fb = fp2;
  lzfb_5968fb = fp1;
  if ((x86::reg8((!(lzfa_5968fb >= lzfb_5968fb) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_5968fb, lzfb_5968fb) ? 0x40 : 0)) != 0)) goto L_0x0059690d;
  eax = x86::reg32(ebx - 0x2u);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_014));
  s_01c = eax;
L_0x0059690d:
  fp2 = x86::Float(rc::ldf32(ecx + 0x14u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x8u)));
  fp3 = x86::Float(rc::ldf32(ecx + 0xcu));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x10u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_59692a = fp2;
  lzfb_59692a = fp1;
  if ((x86::reg8((!(lzfa_59692a >= lzfb_59692a) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_59692a, lzfb_59692a) ? 0x40 : 0)) != 0)) goto L_0x0059693c;
  eax = x86::reg32(ebx - 0x1u);
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_014));
  s_01c = eax;
L_0x0059693c:
  fp2 = x86::Float(rc::ldf32(ecx + 0x24u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x8u)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x1cu));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx + 0x20u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_596959 = fp2;
  lzfb_596959 = fp1;
  if ((x86::reg8((!(lzfa_596959 >= lzfb_596959) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_596959, lzfb_596959) ? 0x40 : 0)) != 0)) goto L_0x00596968;
  fp1 = x86::Float(fp1);
  s_01c = ebx;
  fp1 = x86::Float(fl(s_014));
L_0x00596968:
  eax = s_040;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x40u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596975 = a;
  lzb_596975 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_596975)) < x86::sreg32(x86::reg32(lzb_596975)))) goto L_0x005968b3;
L_0x0059697d:
  ebx = s_040;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596981 = a;
  lzb_596981 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_596981)) >= x86::sreg32(x86::reg32(lzb_596981)))) goto L_0x005969c3;
  ecx = edx;
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  ecx = x86::reg32(ecx + edi + 0x4u);
  edi = edi;
L_0x00596990:
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(ecx));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_5969ac = fp2;
  lzfb_5969ac = fp1;
  if ((x86::reg8((!(lzfa_5969ac >= lzfb_5969ac) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_5969ac, lzfb_5969ac) ? 0x40 : 0)) != 0)) goto L_0x005969bb;
  fp1 = x86::Float(fp1);
  s_01c = edx;
  fp1 = x86::Float(fl(s_014));
L_0x005969bb:
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5969bf = a;
  lzb_5969bf = b;
  }
  if ((x86::sreg32(x86::reg32(lza_5969bf)) < x86::sreg32(x86::reg32(lzb_5969bf)))) goto L_0x00596990;
L_0x005969c3:
  fp2 = x86::Float(rc::ldf32(edi + 0x44u));
  eax = s_034;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5969ca = a;
  lzb_5969ca = b;
  }
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x40u));
  s_028 = 0x0u;
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  edx = 0x1u;
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(edi + 0x48u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  if ((x86::sreg32(x86::reg32(lza_5969ca)) < x86::sreg32(x86::reg32(lzb_5969ca)))) goto L_0x00596ac4;
  ebx = 0x4u;
  ecx = x86::reg32(edi + 0x54u);
L_0x005969fa:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_596a16 = fp3;
  lzfb_596a16 = fp2;
  if (!(lzfa_596a16 < lzfb_596a16)) goto L_0x00596a25;
  fp2 = x86::Float(fp2);
  s_028 = edx;
  fp2 = x86::Float(fl(s_014));
L_0x00596a25:
  fp3 = x86::Float(rc::ldf32(ecx + 0x10u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x14u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0xcu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_596a42 = fp3;
  lzfb_596a42 = fp2;
  if (!(lzfa_596a42 < lzfb_596a42)) goto L_0x00596a54;
  eax = x86::reg32(ebx - 0x2u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_028 = eax;
L_0x00596a54:
  fp3 = x86::Float(rc::ldf32(ecx + 0x20u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x24u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x1cu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_596a71 = fp3;
  lzfb_596a71 = fp2;
  if (!(lzfa_596a71 < lzfb_596a71)) goto L_0x00596a83;
  eax = x86::reg32(ebx - 0x1u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_028 = eax;
L_0x00596a83:
  fp3 = x86::Float(rc::ldf32(ecx + 0x30u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x34u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x2cu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_596aa0 = fp3;
  lzfb_596aa0 = fp2;
  if (!(lzfa_596aa0 < lzfb_596aa0)) goto L_0x00596aaf;
  fp2 = x86::Float(fp2);
  s_028 = ebx;
  fp2 = x86::Float(fl(s_014));
L_0x00596aaf:
  eax = s_04c;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x40u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596abc = a;
  lzb_596abc = b;
  }
  if ((x86::sreg32(x86::reg32(lza_596abc)) < x86::sreg32(x86::reg32(lzb_596abc)))) goto L_0x005969fa;
L_0x00596ac4:
  ebx = s_04c;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596ac8 = a;
  lzb_596ac8 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_596ac8)) >= x86::sreg32(x86::reg32(lzb_596ac8)))) goto L_0x00596b08;
  ecx = edx;
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  ecx = x86::reg32(ecx + edi + 0x44u);
L_0x00596ad5:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(esi + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(ecx)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_596af1 = fp3;
  lzfb_596af1 = fp2;
  if (!(lzfa_596af1 < lzfb_596af1)) goto L_0x00596b00;
  fp2 = x86::Float(fp2);
  s_028 = edx;
  fp2 = x86::Float(fl(s_014));
L_0x00596b00:
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596b04 = a;
  lzb_596b04 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_596b04)) < x86::sreg32(x86::reg32(lzb_596b04)))) goto L_0x00596ad5;
L_0x00596b08:
  cmpa = fp2;
  cmpb = fp1;
  fp1 = x86::Float(fp1);
  if (!(cmpa > cmpb)) goto L_0x00596dc1;
  fp1 = x86::Float(fl(s_070));
  eax = s_02c;
  fp1 = pmul(fp1, Float(fl(s_018)));
  fp2 = x86::Float(fl(s_074));
  fp2 = pmul(Float(fl(s_018)), fp2);
  fp3 = x86::Float(fl(s_078));
  fp3 = pmul(Float(fl(s_018)), fp3);
  s_228 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_1d0 = bits(fp2);
  s_1d4 = bits(fp1);
  fp1 = x86::Float(fl(s_228));
  fp2 = x86::Float(fl(s_1d0));
  fp2 = padd(fp2, Float(rc::ldf32(eax)));
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = padd(Float(fl(s_1d4)), fp3);
  fp4 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = s_024;
  fp4 = padd(fp1, fp4);
  s_248 = bits(fp4);
  edx = s_248;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x80u, float(fp3));
  rc::st32(edi + 0x88u, edx);
  rc::stf32(edi + 0x84u, float(fp2));
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(fl(s_090));
  fp1 = pmul(fp1, Float(fl(s_020)));
  fp2 = x86::Float(fl(s_094));
  fp2 = pmul(Float(fl(s_020)), fp2);
  fp3 = x86::Float(fl(s_098));
  fp3 = pmul(Float(fl(s_020)), fp3);
  s_268 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_190 = bits(fp2);
  s_194 = bits(fp1);
  fp1 = x86::Float(fl(s_268));
  fp2 = x86::Float(fl(s_190));
  fp2 = padd(fp2, Float(rc::ldf32(eax)));
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = padd(Float(fl(s_194)), fp3);
  fp4 = x86::Float(rc::ldf32(eax + 0x8u));
  fp4 = padd(fp1, fp4);
  s_288 = bits(fp4);
  eax = s_288;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x90u, float(fp3));
  rc::st32(edi + 0x98u, eax);
  rc::stf32(edi + 0x94u, float(fp2));
  fp1 = x86::Float(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x90u));
  fp1 = psub(fp1, Float(rc::ldf32(edi + 0x80u)));
  fp2 = x86::Float(rc::ldf32(edi + 0x94u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x84u)));
  fp3 = x86::Float(rc::ldf32(edi + 0x98u));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x88u)));
  fp4 = fp3;
  fp4 = pmul(fp4, fp3);
  fp5 = fp2;
  fp5 = pmul(fp5, fp2);
  fp4 = padd(fp5, fp4);
  fp5 = fp1;
  fp5 = pmul(fp5, fp1);
  fp4 = padd(fp5, fp4);
  fp1 = x86::Float(fp4);
  fp3 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  ecx = s_048;
  eax = s_01c;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596c4e = a;
  lzb_596c4e = b;
  }
  edx = s_044;
  if ((x86::reg32(lza_596c4e) == x86::reg32(lzb_596c4e))) goto L_0x00596c5a;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596c56 = a;
  lzb_596c56 = b;
  }
  if ((x86::reg32(lza_596c56) != x86::reg32(lzb_596c56))) goto L_0x00596c74;
L_0x00596c5a:
  ebx = s_03c;
  eax = s_028;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596c62 = a;
  lzb_596c62 = b;
  }
  if ((x86::reg32(lza_596c62) == x86::reg32(lzb_596c62))) goto L_0x00596e18;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = s_038;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596c6a = a;
  lzb_596c6a = b;
  }
  if ((x86::reg32(lza_596c6a) == x86::reg32(lzb_596c6a))) goto L_0x00596e14;
L_0x00596c74:
  lzfa_596c7a = fp1;
  lzfb_596c7a = x86::Float(fl(s_068));
  if (!(lzfa_596c7a < lzfb_596c7a)) goto L_0x00596dbf;
  fp2 = x86::Float(rc::ldf32(edi + 0x84u));
  eax = rc::ld32(esi);
  s_164 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x88u));
  s_140 = eax;
  eax = rc::ld32(esi + 0x4u);
  s_168 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x94u));
  s_144 = eax;
  eax = rc::ld32(esi + 0x8u);
  s_154 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(edi + 0x98u));
  s_148 = eax;
  eax = rc::ld32(edi + 0x80u);
  s_158 = bits(fp2);
  s_160 = eax;
  eax = rc::ld32(edi + 0x90u);
  s_068 = bits(fp1);
  fp1 = x86::Float(fl(s_018));
  s_150 = eax;
  cmpa = fp1;
  cmpb = kZero;
  ebx = 0x1u;
  s_0cc = ebx;
  if (!(cmpa == cmpb)) goto L_0x00596d18;
  s_13c = ebx;
  goto L_0x00596d4b;
L_0x00596d18:
  fp1 = x86::Float(fl(s_018));
  lzfa_596d24 = fp1;
  lzfb_596d24 = kOne;
  if (!(lzfa_596d24 == lzfb_596d24)) goto L_0x00596d39;
  s_13c = ebx;
  IA[0] = edx;
  goto L_0x00596d52;
L_0x00596d39:
  s_13c = 0x2u;
  IA[1] = edx;
L_0x00596d4b:
  IA[0] = ecx;
L_0x00596d52:
  fp1 = x86::Float(fl(s_020));
  lzfa_596d5e = fp1;
  lzfb_596d5e = kZero;
  if (!(lzfa_596d5e == lzfb_596d5e)) goto L_0x00596d77;
  ecx = s_03c;
  s_128 = ebx;
  IB[0] = ecx;
  goto L_0x00596dc1;
L_0x00596d77:
  fp1 = x86::Float(fl(s_020));
  lzfa_596d83 = fp1;
  lzfb_596d83 = kOne;
  if (!(lzfa_596d83 == lzfb_596d83)) goto L_0x00596d9c;
  edx = s_038;
  s_128 = ebx;
  IB[0] = edx;
  goto L_0x00596dc1;
L_0x00596d9c:
  eax = s_03c;
  ecx = s_038;
  s_128 = 0x2u;
  IB[0] = eax;
  IB[1] = ecx;
  goto L_0x00596dc1;
L_0x00596dbf:
  fp1 = x86::Float(fp1);
L_0x00596dc1:
  eax = s_038;
  ecx = s_04c;
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596dca = a;
  lzb_596dca = b;
  }
  s_038 = eax;
  if ((x86::sreg32(x86::reg32(lza_596dca)) < x86::sreg32(x86::reg32(lzb_596dca)))) goto L_0x005960a0;
L_0x00596dd6:
  eax = s_06c;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = s_034;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596dda = a;
  lzb_596dda = b;
  }
  s_03c = eax;
  if ((x86::sreg32(x86::reg32(lza_596dda)) < x86::sreg32(x86::reg32(lzb_596dda)))) goto L_0x00596010;
L_0x00596de8:
  eax = s_044;
  ecx = s_040;
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596df1 = a;
  lzb_596df1 = b;
  }
  s_044 = eax;
  if ((x86::sreg32(x86::reg32(lza_596df1)) < x86::sreg32(x86::reg32(lzb_596df1)))) goto L_0x00595f96;
L_0x00596dfd:
  eax = s_064;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = s_030;
  const x86::reg32 r = x86::reg32(a - b);
  lza_596e01 = a;
  lzb_596e01 = b;
  }
  s_048 = eax;
  if ((x86::sreg32(x86::reg32(lza_596e01)) < x86::sreg32(x86::reg32(lzb_596e01)))) goto L_0x00595f67;
  app->allowPreemption(cpu);
  goto L_0x00595681;
L_0x00596e14:
  ebx = s_03c;
L_0x00596e18:
  fp2 = x86::Float(fl(s_018));
  esi = 0x1u;
  lzfa_596e29 = fp2;
  lzfb_596e29 = kZero;
  if (!(lzfa_596e29 == lzfb_596e29)) goto L_0x00596e5d;
  rc::st32(edi + 0xc0u, esi);
  edx = rc::ld32(edi + ecx * 4 + 0xb0u);
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  rc::st32(edi + 0xb0u, edx);
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  eax = rc::ld32(ecx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi, eax);
  rc::stf32(edi + 0x4u, float(fp3));
  rc::stf32(edi + 0x8u, float(fp2));
  goto L_0x00596ef1;
L_0x00596e5d:
  fp2 = x86::Float(fl(s_018));
  lzfa_596e69 = fp2;
  lzfb_596e69 = kOne;
  if (!(lzfa_596e69 == lzfb_596e69)) goto L_0x00596e9a;
  rc::st32(edi + 0xc0u, esi);
  ecx = rc::ld32(edi + edx * 4 + 0xb0u);
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  rc::st32(edi + 0xb0u, ecx);
  fp2 = x86::Float(rc::ldf32(edx + 0x4u));
  fp3 = x86::Float(rc::ldf32(edx + 0x8u));
  edx = rc::ld32(edx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi, edx);
  rc::stf32(edi + 0x4u, float(fp3));
  rc::stf32(edi + 0x8u, float(fp2));
  goto L_0x00596ef1;
L_0x00596e9a:
  rc::st32(edi + 0xc0u, 0x2u);
  eax = rc::ld32(edi + ecx * 4 + 0xb0u);
  rc::st32(edi + 0xb0u, eax);
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  fp2 = x86::Float(rc::ldf32(ecx + edi + 0x4u));
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  ecx = rc::ld32(ecx);
  rc::st32(edi, ecx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x4u, float(fp3));
  rc::stf32(edi + 0x8u, float(fp2));
  eax = rc::ld32(edi + edx * 4 + 0xb0u);
  rc::st32(edi + 0xb4u, eax);
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  fp2 = x86::Float(rc::ldf32(edx + edi + 0x4u));
  ecx = rc::ld32(edx + edi);
  fp3 = x86::Float(rc::ldf32(edx + edi + 0x8u));
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x10u, ecx);
  rc::stf32(edi + 0x14u, float(fp3));
  rc::stf32(edi + 0x18u, float(fp2));
L_0x00596ef1:
  fp2 = x86::Float(fl(s_020));
  lzfa_596efd = fp2;
  lzfb_596efd = kZero;
  if (!(lzfa_596efd == lzfb_596efd)) goto L_0x00596f3a;
  rc::st32(edi + 0xd4u, esi);
  edx = rc::ld32(edi + ebx * 4 + 0xc4u);
  rc::st32(edi + 0xc4u, edx);
  eax = x86::reg32(ebx + 0x4u);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = rc::ld32(eax);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x40u, eax);
  rc::stf32(edi + 0x44u, float(fp3));
  rc::stf32(edi + 0x48u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x00596f3a:
  fp2 = x86::Float(fl(s_020));
  lzfa_596f46 = fp2;
  lzfb_596f46 = kOne;
  if (!(lzfa_596f46 == lzfb_596f46)) goto L_0x00596f89;
  eax = s_038;
  rc::st32(edi + 0xd4u, esi);
L_0x00596f55:
  ecx = rc::ld32(edi + eax * 4 + 0xc4u);
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
L_0x00596f5f:
  rc::st32(edi + 0xc4u, ecx);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  edx = rc::ld32(eax + edi);
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  rc::st32(edi + 0x40u, edx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x44u, float(fp3));
  rc::stf32(edi + 0x48u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x00596f89:
  rc::st32(edi + 0xd4u, 0x2u);
  eax = rc::ld32(edi + ebx * 4 + 0xc4u);
  rc::st32(edi + 0xc4u, eax);
  eax = s_038;
  ecx = rc::ld32(edi + eax * 4 + 0xc4u);
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 a = ebx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ebx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  rc::st32(edi + 0xc8u, ecx);
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  edx = rc::ld32(ebx);
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  app->allowPreemption(cpu);
  goto L_0x0059590a;
L_0x00596fc9:
  ecx = s_0b0;
  edx = s_0b4;
  eax = s_0b8;
  rc::st32(esi, ecx);
  rc::st32(esi + 0x4u, edx);
L_0x00596fe3:
  fp1 = x86::Float(rc::ldf32(esi + 0x4u));
  ecx = s_048;
  rc::st32(esi + 0x8u, eax);
  fp1 = pmul(fp1, Float(rc::ldf32(ebx + 0x4u)));
  fp2 = x86::Float(rc::ldf32(ebx));
  eax = s_040;
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  s_01c = ecx;
  ecx = s_018;
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597000 = a;
  lzb_597000 = b;
  }
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x8u)));
  fp1 = padd(fp1, fp2);
  s_024 = bits(fp1);
  if ((x86::sreg32(x86::reg32(lza_597000)) >= x86::sreg32(x86::reg32(lzb_597000)))) goto L_0x00597043;
  eax = s_020;
  fp2 = x86::Float(rc::ldf32(eax - 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(esi + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(eax)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_597034 = fp2;
  lzfb_597034 = x86::Float(fl(s_024));
  if ((x86::reg8((!(lzfa_597034 >= lzfb_597034) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597034, lzfb_597034) ? 0x40 : 0)) != 0)) goto L_0x00597043;
  fp1 = x86::Float(fp1);
  s_01c = ecx;
  fp1 = x86::Float(fl(s_014));
L_0x00597043:
  fp2 = x86::Float(rc::ldf32(edi + 0x48u));
  eax = s_04c;
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x8u)));
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_59704e = a;
  lzb_59704e = b;
  }
  fp3 = x86::Float(rc::ldf32(edi + 0x44u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  s_028 = 0x0u;
  edx = 0x1u;
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(edi + 0x40u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp2 = padd(fp2, fp3);
  s_06c = bits(fp2);
  if ((x86::sreg32(x86::reg32(lza_59704e)) < x86::sreg32(x86::reg32(lzb_59704e)))) goto L_0x0059714d;
  ebx = 0x4u;
  ecx = x86::reg32(edi + 0x54u);
L_0x0059707f:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(esi + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(ecx)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_59709b = fp3;
  lzfb_59709b = fp2;
  if (!(lzfa_59709b < lzfb_59709b)) goto L_0x005970aa;
  fp2 = x86::Float(fp2);
  s_028 = edx;
  fp2 = x86::Float(fl(s_014));
L_0x005970aa:
  fp3 = x86::Float(rc::ldf32(ecx + 0x14u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0xcu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x10u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_5970c7 = fp3;
  lzfb_5970c7 = fp2;
  if (!(lzfa_5970c7 < lzfb_5970c7)) goto L_0x005970d9;
  eax = x86::reg32(ebx - 0x2u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_028 = eax;
L_0x005970d9:
  fp3 = x86::Float(rc::ldf32(ecx + 0x24u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x1cu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x20u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_5970f6 = fp3;
  lzfb_5970f6 = fp2;
  if (!(lzfa_5970f6 < lzfb_5970f6)) goto L_0x00597108;
  eax = x86::reg32(ebx - 0x1u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_028 = eax;
L_0x00597108:
  fp3 = x86::Float(rc::ldf32(ecx + 0x34u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x2cu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x30u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_597125 = fp3;
  lzfb_597125 = fp2;
  if (!(lzfa_597125 < lzfb_597125)) goto L_0x00597134;
  fp2 = x86::Float(fp2);
  s_028 = ebx;
  fp2 = x86::Float(fl(s_014));
L_0x00597134:
  eax = s_04c;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x40u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597141 = a;
  lzb_597141 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_597141)) < x86::sreg32(x86::reg32(lzb_597141)))) goto L_0x0059707f;
  s_06c = bits(fp2);
L_0x0059714d:
  ebx = s_04c;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597151 = a;
  lzb_597151 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_597151)) >= x86::sreg32(x86::reg32(lzb_597151)))) goto L_0x00597197;
  ecx = edx;
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  ecx = x86::reg32(ecx + edi + 0x44u);
  edi = edi;
L_0x00597160:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_59717c = fp3;
  lzfb_59717c = fp2;
  if (!(lzfa_59717c < lzfb_59717c)) goto L_0x0059718b;
  fp2 = x86::Float(fp2);
  s_028 = edx;
  fp2 = x86::Float(fl(s_014));
L_0x0059718b:
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_59718f = a;
  lzb_59718f = b;
  }
  if ((x86::sreg32(x86::reg32(lza_59718f)) < x86::sreg32(x86::reg32(lzb_59718f)))) goto L_0x00597160;
  s_06c = bits(fp2);
L_0x00597197:
  cmpa = fp2;
  cmpb = fp1;
  fp1 = x86::Float(fp1);
  if (!(cmpa > cmpb)) goto L_0x00597585;
  edx = s_028;
  eax = s_02c;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  fp1 = x86::Float(rc::ldf32(edx + edi));
  ebx = x86::reg32(edx + edi);
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax + 0x8u)));
  s_228 = bits(fp3);
  ecx = s_228;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_050 = bits(fp2);
  s_058 = ecx;
  ecx = s_034;
  s_054 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(eax + 0x4u));
  fp2 = psub(Float(rc::ldf32(ecx + 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  fp3 = psub(Float(rc::ldf32(ecx + 0x8u)), fp3);
  s_208 = bits(fp3);
  edx = s_208;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0f8 = edx;
  s_0f0 = bits(fp2);
  s_0f4 = bits(fp1);
  fp1 = x86::Float(fl(s_0f0));
  fp1 = pmul(fp1, Float(fl(s_0b8)));
  fp2 = x86::Float(fl(s_0f8));
  fp2 = pmul(fp2, Float(fl(s_0b0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_0f4));
  fp2 = pmul(Float(fl(s_0b0)), fp2);
  fp3 = x86::Float(fl(s_0b4));
  fp3 = pmul(Float(fl(s_0f0)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_0f8));
  fp3 = pmul(fp3, Float(fl(s_0b4)));
  fp4 = x86::Float(fl(s_0f4));
  fp4 = pmul(Float(fl(s_0b8)), fp4);
  fp3 = psub(fp3, fp4);
  s_100 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_104 = bits(fp2);
  s_108 = bits(fp1);
L_ret_597235:
  fp1 = x86::Float(fl(s_108));
  fp1 = pmul(fp1, Float(fl(s_058)));
  fp2 = x86::Float(fl(s_104));
  fp2 = pmul(fp2, Float(fl(s_054)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_100));
  fp2 = pmul(fp2, Float(fl(s_050)));
  fp1 = padd(fp1, fp2);
  lzfa_59726a = fp1;
  lzfb_59726a = kZero;
  if ((x86::reg8((!(lzfa_59726a >= lzfb_59726a) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_59726a, lzfb_59726a) ? 0x40 : 0)) != 0)) goto L_0x00597585;
  eax = s_034;
  fp1 = x86::Float(rc::ldf32(ebx));
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax + 0x8u)));
  s_1f8 = bits(fp3);
  ecx = s_1f8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_050 = bits(fp2);
  s_058 = ecx;
  ecx = s_030;
  s_054 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(ecx - 0x8u));
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx));
  fp3 = psub(fp3, Float(rc::ldf32(eax + 0x8u)));
  s_278 = bits(fp3);
  edx = s_278;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0f8 = edx;
  s_0f0 = bits(fp2);
  s_0f4 = bits(fp1);
  fp1 = x86::Float(fl(s_0f0));
  fp1 = pmul(fp1, Float(fl(s_0b8)));
  fp2 = x86::Float(fl(s_0f8));
  fp2 = pmul(fp2, Float(fl(s_0b0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_0f4));
  fp2 = pmul(Float(fl(s_0b0)), fp2);
  fp3 = x86::Float(fl(s_0b4));
  fp3 = pmul(Float(fl(s_0f0)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_0f8));
  fp3 = pmul(fp3, Float(fl(s_0b4)));
  fp4 = x86::Float(fl(s_0f4));
  fp4 = pmul(Float(fl(s_0b8)), fp4);
  fp3 = psub(fp3, fp4);
  s_100 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_104 = bits(fp2);
  s_108 = bits(fp1);
L_ret_5972f4:
  fp1 = x86::Float(fl(s_108));
  fp1 = pmul(fp1, Float(fl(s_058)));
  fp2 = x86::Float(fl(s_104));
  fp2 = pmul(fp2, Float(fl(s_054)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_100));
  fp2 = pmul(fp2, Float(fl(s_050)));
  fp1 = padd(fp1, fp2);
  lzfa_597329 = fp1;
  lzfb_597329 = kZero;
  if ((x86::reg8((!(lzfa_597329 >= lzfb_597329) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597329, lzfb_597329) ? 0x40 : 0)) != 0)) goto L_0x00597585;
  eax = s_030;
  fp1 = x86::Float(rc::ldf32(ebx));
  fp1 = psub(fp1, Float(rc::ldf32(eax - 0x8u)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax - 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax)));
  s_238 = bits(fp3);
  ecx = s_238;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_050 = bits(fp2);
  s_058 = ecx;
  ecx = s_02c;
  s_054 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = psub(fp1, Float(rc::ldf32(eax - 0x8u)));
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax - 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax)));
  s_298 = bits(fp3);
  edx = s_298;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0f8 = edx;
  s_0f0 = bits(fp2);
  s_0f4 = bits(fp1);
  fp1 = x86::Float(fl(s_0f0));
  fp1 = pmul(fp1, Float(fl(s_0b8)));
  fp2 = x86::Float(fl(s_0f8));
  fp2 = pmul(fp2, Float(fl(s_0b0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_0f4));
  fp2 = pmul(Float(fl(s_0b0)), fp2);
  fp3 = x86::Float(fl(s_0b4));
  fp3 = pmul(Float(fl(s_0f0)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_0f8));
  fp3 = pmul(fp3, Float(fl(s_0b4)));
  fp4 = x86::Float(fl(s_0f4));
  fp4 = pmul(Float(fl(s_0b8)), fp4);
  fp3 = psub(fp3, fp4);
  s_100 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_104 = bits(fp2);
  s_108 = bits(fp1);
L_ret_5973b3:
  fp1 = x86::Float(fl(s_108));
  fp1 = pmul(fp1, Float(fl(s_058)));
  fp2 = x86::Float(fl(s_104));
  fp2 = pmul(fp2, Float(fl(s_054)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_100));
  fp2 = pmul(fp2, Float(fl(s_050)));
  fp1 = padd(fp1, fp2);
  lzfa_5973e8 = fp1;
  lzfb_5973e8 = kZero;
  if ((x86::reg8((!(lzfa_5973e8 >= lzfb_5973e8) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_5973e8, lzfb_5973e8) ? 0x40 : 0)) != 0)) goto L_0x00597585;
  fp1 = x86::Float(fl(s_06c));
  fp1 = psub(fp1, Float(fl(s_024)));
  fp2 = fp1;
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = fp1;
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  {
  const x86::Float t = fp3;
  fp3 = fp1;
  fp1 = t;
  }
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  s_218 = bits(fp3);
  s_250 = bits(fp2);
  eax = s_250;
  fp2 = x86::Float(fl(s_218));
  s_0a0 = eax;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  eax = s_01c;
  s_0a4 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(ebx));
  fp2 = psub(fp2, Float(fl(s_0a0)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp3 = psub(fp3, Float(fl(s_0a4)));
  fp4 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp4 = psub(fp4, fp1);
  s_2a8 = bits(fp4);
  ecx = s_2a8;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x80u, float(fp3));
  rc::stf32(edi + 0x84u, float(fp2));
  rc::st32(edi + 0x88u, ecx);
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  edx = rc::ld32(ebx);
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  ecx = s_048;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_59747d = a;
  lzb_59747d = b;
  }
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x90u, edx);
  rc::stf32(edi + 0x94u, float(fp3));
  rc::stf32(edi + 0x98u, float(fp2));
  fp2 = fp1;
  fp1 = pmul(fp1, fp2);
  fp2 = x86::Float(fl(s_0a4));
  fp2 = pmul(fp2, Float(fl(s_0a4)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_0a0));
  fp2 = pmul(fp2, Float(fl(s_0a0)));
  fp1 = padd(fp1, fp2);
  if ((x86::reg32(lza_59747d) == x86::reg32(lzb_59747d))) goto L_0x005977f2;
  lzfa_5974c3 = fp1;
  lzfb_5974c3 = x86::Float(fl(s_068));
  if (!(lzfa_5974c3 < lzfb_5974c3)) goto L_0x00597583;
  edx = rc::ld32(edi + 0x80u);
  s_068 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x84u));
  s_160 = edx;
  edx = rc::ld32(edi + 0x90u);
  s_150 = edx;
  edx = rc::ld32(esi);
  s_164 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x88u));
  s_140 = edx;
  edx = rc::ld32(esi + 0x4u);
  s_168 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x94u));
  eax = 0x1u;
  s_144 = edx;
  edx = rc::ld32(esi + 0x8u);
  s_154 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x98u));
  s_0cc = eax;
  s_148 = edx;
  edx = s_03c;
  IA[0] = ecx;
  ecx = s_044;
  s_128 = eax;
  eax = s_028;
  s_158 = bits(fp1);
  s_13c = 0x3u;
  IA[1] = ecx;
  IA[2] = edx;
  IB[0] = eax;
  goto L_0x00597585;
L_0x00597583:
  fp1 = x86::Float(fp1);
L_0x00597585:
  edx = s_020;
  eax = s_03c;
  ebx = s_018;
  ecx = 0x10u;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  edx = r;
  }
  s_020 = edx;
  edx = s_030;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  ecx = s_040;
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  {
  const x86::reg32 r = x86::reg32(ebx - 1);
  ebx = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5975a8 = a;
  lzb_5975a8 = b;
  }
  s_03c = eax;
  s_018 = ebx;
  s_030 = edx;
  if ((x86::sreg32(x86::reg32(lza_5975a8)) < x86::sreg32(x86::reg32(lzb_5975a8)))) goto L_0x00595730;
L_0x005975bc:
  ecx = s_044;
  edx = s_038;
  eax = s_088;
  ebx = s_034;
  {
  const x86::reg32 r = x86::reg32(ecx + 1);
  ecx = r;
  }
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  s_088 = eax;
  eax = s_040;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5975e1 = a;
  lzb_5975e1 = b;
  }
  s_044 = ecx;
  s_038 = edx;
  s_034 = ebx;
  if ((x86::sreg32(x86::reg32(lza_5975e1)) < x86::sreg32(x86::reg32(lzb_5975e1)))) goto L_0x005956e1;
L_0x005975f5:
  ecx = s_02c;
  ebx = s_08c;
  edx = s_064;
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  s_02c = ecx;
  ecx = s_040;
  {
  const x86::reg32 r = x86::reg32(ebx - 1);
  ebx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0xfffffffeu;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597613 = a;
  lzb_597613 = b;
  }
  s_048 = edx;
  s_08c = ebx;
  if ((x86::sreg32(x86::reg32(lza_597613)) < x86::sreg32(x86::reg32(lzb_597613)))) goto L_0x005956a8;
L_0x00597626:
  eax = s_04c;
  ecx = x86::reg32(eax - 0x2u);
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a & b);
  lzr_59762d = r;
  }
  s_048 = 0x0u;
  if ((x86::sreg32(x86::reg32(lzr_59762d)) <= 0)) goto L_0x00597ef5;
  ecx = x86::reg32(edi + 0x40u);
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  s_02c = ecx;
  s_018 = 0x5u;
  ecx = x86::reg32(ecx);
L_0x00597650:
  ecx = s_048;
  {
  const x86::reg32 r = x86::reg32(ecx + 1);
  ecx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597655 = a;
  lzb_597655 = b;
  }
  s_064 = ecx;
  s_044 = ecx;
  if ((x86::sreg32(x86::reg32(lza_597655)) >= x86::sreg32(x86::reg32(lzb_597655)))) goto L_0x00597eca;
  eax = ecx;
  ecx = s_02c;
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  s_038 = eax;
  eax = s_018;
  s_034 = ecx;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = s_048;
  const x86::reg32 r = x86::reg32(a - b);
  eax = r;
  }
  s_020 = eax;
L_0x00597683:
  eax = s_038;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = s_04c;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597687 = a;
  lzb_597687 = b;
  }
  s_03c = eax;
  if ((x86::sreg32(x86::reg32(lza_597687)) >= x86::sreg32(x86::reg32(lzb_597687)))) goto L_0x00597e97;
  ecx = s_038;
  eax = s_020;
  edx = s_034;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  eax = r;
  }
  s_06c = eax;
  eax = s_064;
  ecx = x86::reg32(eax + ecx * 2 - 0x2u);
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  eax = edi;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x18u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0xa4u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  s_030 = edx;
  s_088 = eax;
L_0x005976d0:
  eax = s_034;
  ebx = s_02c;
  fp1 = x86::Float(rc::ldf32(eax));
  fp1 = psub(fp1, Float(rc::ldf32(ebx)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(Float(rc::ldf32(eax + 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(Float(rc::ldf32(eax + 0x8u)), fp3);
  eax = s_030;
  s_288 = bits(fp3);
  edx = s_288;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0e0 = bits(fp2);
  s_0e8 = edx;
  s_0e4 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(eax - 0x8u));
  fp1 = psub(fp1, Float(rc::ldf32(ebx)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(Float(rc::ldf32(eax - 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(Float(rc::ldf32(eax)), fp3);
  s_268 = bits(fp3);
  eax = s_268;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_188 = eax;
  s_180 = bits(fp2);
  s_184 = bits(fp1);
  fp1 = x86::Float(fl(s_180));
  fp1 = pmul(fp1, Float(fl(s_0e8)));
  fp2 = x86::Float(fl(s_188));
  fp2 = pmul(fp2, Float(fl(s_0e0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_184));
  fp2 = pmul(Float(fl(s_0e0)), fp2);
  fp3 = x86::Float(fl(s_0e4));
  fp3 = pmul(Float(fl(s_180)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_188));
  fp3 = pmul(fp3, Float(fl(s_0e4)));
  fp4 = x86::Float(fl(s_184));
  fp4 = pmul(Float(fl(s_0e8)), fp4);
  fp3 = psub(fp3, fp4);
  s_0b0 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0b4 = bits(fp2);
  s_0b8 = bits(fp1);
L_ret_59775e:
  fp1 = x86::Float(fl(s_0b8));
  fp2 = x86::Float(fl(s_0b4));
  fp3 = x86::Float(fl(s_0b0));
  fp4 = fp3;
  fp4 = pmul(fp4, fp3);
  fp5 = fp2;
  fp5 = pmul(fp5, fp2);
  fp4 = padd(fp5, fp4);
  fp5 = fp1;
  fp5 = pmul(fp5, fp1);
  fp4 = padd(fp5, fp4);
  fp4 = cpu.fpu.sqrt(fp4);
  fp1 = x86::Float(fp4);
  fp3 = x86::Float(fp3);
  fp2 = x86::Float(fp2);
  lzfa_43cf3c = fp1;
  lzfb_43cf3c = kZero;
  if ((lzfa_43cf3c == lzfb_43cf3c)) goto L_0x0043cf6c_597773;
  fp1 = pdiv(kOne, fp1);
  eax = s_0b8;
  s_m10 = eax;
  fp2 = x86::Float(fl(s_0b4));
  fp3 = fp1;
  fp3 = pmul(fp3, Float(fl(s_0b0)));
  s_0b0 = bits(fp3);
  fp2 = pmul(fp1, fp2);
  s_0b4 = bits(fp2);
  fp2 = x86::Float(fl(s_m10));
  fp2 = pmul(fp1, fp2);
  s_0b8 = bits(fp2);
  fp1 = x86::Float(fp1);
  goto L_ret_597773;
L_0x0043cf6c_597773:
  fp1 = x86::Float(fp1);
  ecx = 0;
  s_0b0 = 0x3f800000u;
  s_0b4 = ecx;
  s_0b8 = ecx;
L_ret_597773:
  fp1 = x86::Float(rc::ldf32(ebx));
  fp1 = psub(fp1, Float(rc::ldf32(edi)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(edi + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(edi + 0x8u)));
  fp4 = x86::Float(fl(s_0b8));
  fp4 = pmul(fp4, fp3);
  {
  const x86::Float t = fp4;
  fp4 = fp2;
  fp2 = t;
  }
  fp4 = pmul(fp4, Float(fl(s_0b4)));
  fp2 = padd(fp2, fp4);
  {
  const x86::Float t = fp3;
  fp3 = fp1;
  fp1 = t;
  }
  fp3 = pmul(fp3, Float(fl(s_0b0)));
  fp2 = padd(fp2, fp3);
  cmpa = fp2;
  cmpb = kZero;
  fp1 = x86::Float(fp1);
  if (!(cmpa < cmpb)) goto L_0x0059788e;
  fp1 = x86::Float(fl(s_0b0));
  fp1 = pneg(fp1);
  fp2 = x86::Float(fl(s_0b4));
  fp2 = pneg(fp2);
  fp3 = x86::Float(fl(s_0b8));
  fp3 = pneg(fp3);
  s_248 = bits(fp3);
  eax = s_248;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::stf32(esi, float(fp2));
  rc::stf32(esi + 0x4u, float(fp1));
  goto L_0x005978a8;
L_0x005977f2:
  rc::st32(edi + 0xc0u, 0x3u);
  edx = rc::ld32(edi + ecx * 4 + 0xb0u);
  rc::st32(edi + 0xb0u, edx);
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  fp2 = x86::Float(rc::ldf32(ecx + edi + 0x4u));
  eax = rc::ld32(ecx + edi);
  fp3 = x86::Float(rc::ldf32(ecx + edi + 0x8u));
  rc::st32(edi, eax);
  eax = s_044;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x4u, float(fp3));
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  rc::stf32(edi + 0x8u, float(fp2));
  ecx = rc::ld32(edi + eax * 4 + 0xb0u);
  rc::st32(edi + 0xb4u, ecx);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  edx = rc::ld32(eax + edi);
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  eax = s_03c;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x10u, edx);
  rc::stf32(edi + 0x14u, float(fp3));
  rc::stf32(edi + 0x18u, float(fp2));
  ecx = rc::ld32(edi + eax * 4 + 0xb0u);
  rc::st32(edi + 0xb8u, ecx);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  edx = rc::ld32(eax + edi);
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  eax = s_028;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x20u, edx);
  rc::stf32(edi + 0x24u, float(fp3));
  rc::stf32(edi + 0x28u, float(fp2));
  rc::st32(edi + 0xd4u, 0x1u);
  app->allowPreemption(cpu);
  goto L_0x00596f55;
L_0x0059788e:
  ecx = s_0b0;
  edx = s_0b4;
  eax = s_0b8;
  rc::st32(esi, ecx);
  rc::st32(esi + 0x4u, edx);
L_0x005978a8:
  ecx = s_048;
  rc::st32(esi + 0x8u, eax);
  fp1 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp1 = pmul(Float(rc::ldf32(esi + 0x8u)), fp1);
  eax = s_04c;
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  s_028 = ecx;
  fp2 = pmul(fp2, Float(rc::ldf32(esi + 0x4u)));
  ecx = s_06c;
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_5978c7 = a;
  lzb_5978c7 = b;
  }
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(rc::ldf32(ebx));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp1 = padd(fp1, fp2);
  s_024 = bits(fp1);
  if ((x86::sreg32(x86::reg32(lza_5978c7)) >= x86::sreg32(x86::reg32(lzb_5978c7)))) goto L_0x0059790b;
  eax = s_088;
  fp2 = x86::Float(rc::ldf32(eax - 0x4u));
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = x86::Float(rc::ldf32(eax + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(eax));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_014 = bits(fp2);
  lzfa_5978fc = fp2;
  lzfb_5978fc = x86::Float(fl(s_024));
  if (!(lzfa_5978fc < lzfb_5978fc)) goto L_0x0059790b;
  fp1 = x86::Float(fp1);
  s_028 = ecx;
  fp1 = x86::Float(fl(s_014));
L_0x0059790b:
  fp2 = x86::Float(rc::ldf32(esi));
  eax = s_040;
  fp2 = pmul(fp2, Float(rc::ldf32(edi)));
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597914 = a;
  lzb_597914 = b;
  }
  fp3 = x86::Float(rc::ldf32(esi + 0x8u));
  fp3 = pmul(fp3, Float(rc::ldf32(edi + 0x8u)));
  s_01c = 0x0u;
  edx = 0x1u;
  fp2 = padd(fp2, fp3);
  fp3 = x86::Float(rc::ldf32(esi + 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(edi + 0x4u)));
  fp2 = padd(fp2, fp3);
  s_08c = bits(fp2);
  if ((x86::sreg32(x86::reg32(lza_597914)) < x86::sreg32(x86::reg32(lzb_597914)))) goto L_0x00597a1a;
  ebx = 0x4u;
  ecx = x86::reg32(edi + 0x24u);
L_0x00597949:
  fp3 = x86::Float(rc::ldf32(ecx - 0x14u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx - 0xcu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx - 0x10u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_597966 = fp3;
  lzfb_597966 = fp2;
  if ((x86::reg8((!(lzfa_597966 >= lzfb_597966) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597966, lzfb_597966) ? 0x40 : 0)) != 0)) goto L_0x00597975;
  fp2 = x86::Float(fp2);
  s_01c = edx;
  fp2 = x86::Float(fl(s_014));
L_0x00597975:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(esi + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(ecx)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_597991 = fp3;
  lzfb_597991 = fp2;
  if ((x86::reg8((!(lzfa_597991 >= lzfb_597991) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597991, lzfb_597991) ? 0x40 : 0)) != 0)) goto L_0x005979a3;
  eax = x86::reg32(ebx - 0x2u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_01c = eax;
L_0x005979a3:
  fp3 = x86::Float(rc::ldf32(ecx + 0x14u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0xcu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x10u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_5979c0 = fp3;
  lzfb_5979c0 = fp2;
  if ((x86::reg8((!(lzfa_5979c0 >= lzfb_5979c0) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_5979c0, lzfb_5979c0) ? 0x40 : 0)) != 0)) goto L_0x005979d2;
  eax = x86::reg32(ebx - 0x1u);
  fp2 = x86::Float(fp2);
  fp2 = x86::Float(fl(s_014));
  s_01c = eax;
L_0x005979d2:
  fp3 = x86::Float(rc::ldf32(ecx + 0x24u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x1cu));
  fp4 = pmul(fp4, Float(rc::ldf32(esi)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx + 0x20u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_5979ef = fp3;
  lzfb_5979ef = fp2;
  if ((x86::reg8((!(lzfa_5979ef >= lzfb_5979ef) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_5979ef, lzfb_5979ef) ? 0x40 : 0)) != 0)) goto L_0x005979fe;
  fp2 = x86::Float(fp2);
  s_01c = ebx;
  fp2 = x86::Float(fl(s_014));
L_0x005979fe:
  eax = s_040;
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  ebx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x40u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597a0b = a;
  lzb_597a0b = b;
  }
  if ((x86::sreg32(x86::reg32(lza_597a0b)) < x86::sreg32(x86::reg32(lzb_597a0b)))) goto L_0x00597949;
  s_08c = bits(fp2);
L_0x00597a1a:
  ebx = s_040;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597a1e = a;
  lzb_597a1e = b;
  }
  if ((x86::sreg32(x86::reg32(lza_597a1e)) >= x86::sreg32(x86::reg32(lzb_597a1e)))) goto L_0x00597a6a;
  ecx = edx;
  {
  const x86::reg32 a = ecx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  ecx = r;
  }
  ecx = x86::reg32(ecx + edi + 0x4u);
  goto L_0x00597a30;
L_0x00597a30:
  fp3 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp3 = pmul(fp3, Float(rc::ldf32(esi)));
  fp4 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x8u)));
  fp3 = padd(fp3, fp4);
  fp4 = x86::Float(rc::ldf32(ecx));
  fp4 = pmul(fp4, Float(rc::ldf32(esi + 0x4u)));
  fp3 = padd(fp3, fp4);
  s_014 = bits(fp3);
  lzfa_597a4c = fp3;
  lzfb_597a4c = fp2;
  if ((x86::reg8((!(lzfa_597a4c >= lzfb_597a4c) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597a4c, lzfb_597a4c) ? 0x40 : 0)) != 0)) goto L_0x00597a5b;
  fp2 = x86::Float(fp2);
  s_01c = edx;
  fp2 = x86::Float(fl(s_014));
L_0x00597a5b:
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ebx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597a5f = a;
  lzb_597a5f = b;
  }
  if ((x86::sreg32(x86::reg32(lza_597a5f)) < x86::sreg32(x86::reg32(lzb_597a5f)))) goto L_0x00597a30;
  s_08c = bits(fp2);
L_0x00597a6a:
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  cmpa = fp2;
  cmpb = fp1;
  fp1 = x86::Float(fp1);
  if (!(cmpa > cmpb)) goto L_0x00597e5a;
  edx = s_01c;
  eax = s_02c;
  {
  const x86::reg32 a = edx;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  edx = r;
  }
  fp1 = x86::Float(rc::ldf32(edx + edi));
  ebx = x86::reg32(edx + edi);
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax + 0x8u)));
  s_228 = bits(fp3);
  ecx = s_228;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_050 = bits(fp2);
  s_058 = ecx;
  ecx = s_034;
  s_054 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(eax + 0x4u));
  fp2 = psub(Float(rc::ldf32(ecx + 0x4u)), fp2);
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  fp3 = psub(Float(rc::ldf32(ecx + 0x8u)), fp3);
  s_208 = bits(fp3);
  edx = s_208;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0f8 = edx;
  s_0f0 = bits(fp2);
  s_0f4 = bits(fp1);
  fp1 = x86::Float(fl(s_0f0));
  fp1 = pmul(fp1, Float(fl(s_0b8)));
  fp2 = x86::Float(fl(s_0f8));
  fp2 = pmul(fp2, Float(fl(s_0b0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_0f4));
  fp2 = pmul(Float(fl(s_0b0)), fp2);
  fp3 = x86::Float(fl(s_0b4));
  fp3 = pmul(Float(fl(s_0f0)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_0f8));
  fp3 = pmul(fp3, Float(fl(s_0b4)));
  fp4 = x86::Float(fl(s_0f4));
  fp4 = pmul(Float(fl(s_0b8)), fp4);
  fp3 = psub(fp3, fp4);
  s_100 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_104 = bits(fp2);
  s_108 = bits(fp1);
L_ret_597b07:
  fp1 = x86::Float(fl(s_108));
  fp1 = pmul(fp1, Float(fl(s_058)));
  fp2 = x86::Float(fl(s_104));
  fp2 = pmul(fp2, Float(fl(s_054)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_100));
  fp2 = pmul(fp2, Float(fl(s_050)));
  fp1 = padd(fp1, fp2);
  lzfa_597b3c = fp1;
  lzfb_597b3c = kZero;
  if ((x86::reg8((!(lzfa_597b3c >= lzfb_597b3c) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597b3c, lzfb_597b3c) ? 0x40 : 0)) != 0)) goto L_0x00597e5a;
  eax = s_034;
  fp1 = x86::Float(rc::ldf32(ebx));
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax + 0x8u)));
  s_1f8 = bits(fp3);
  ecx = s_1f8;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_050 = bits(fp2);
  s_058 = ecx;
  ecx = s_030;
  s_054 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(ecx - 0x8u));
  fp1 = psub(fp1, Float(rc::ldf32(eax)));
  fp2 = x86::Float(rc::ldf32(ecx - 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax + 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx));
  fp3 = psub(fp3, Float(rc::ldf32(eax + 0x8u)));
  s_278 = bits(fp3);
  edx = s_278;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0f8 = edx;
  s_0f0 = bits(fp2);
  s_0f4 = bits(fp1);
  fp1 = x86::Float(fl(s_0f0));
  fp1 = pmul(fp1, Float(fl(s_0b8)));
  fp2 = x86::Float(fl(s_0f8));
  fp2 = pmul(fp2, Float(fl(s_0b0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_0f4));
  fp2 = pmul(Float(fl(s_0b0)), fp2);
  fp3 = x86::Float(fl(s_0b4));
  fp3 = pmul(Float(fl(s_0f0)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_0f8));
  fp3 = pmul(fp3, Float(fl(s_0b4)));
  fp4 = x86::Float(fl(s_0f4));
  fp4 = pmul(Float(fl(s_0b8)), fp4);
  fp3 = psub(fp3, fp4);
  s_100 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_104 = bits(fp2);
  s_108 = bits(fp1);
L_ret_597bc6:
  fp1 = x86::Float(fl(s_108));
  fp1 = pmul(fp1, Float(fl(s_058)));
  fp2 = x86::Float(fl(s_104));
  fp2 = pmul(fp2, Float(fl(s_054)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_100));
  fp2 = pmul(fp2, Float(fl(s_050)));
  fp1 = padd(fp1, fp2);
  lzfa_597bfb = fp1;
  lzfb_597bfb = kZero;
  if ((x86::reg8((!(lzfa_597bfb >= lzfb_597bfb) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597bfb, lzfb_597bfb) ? 0x40 : 0)) != 0)) goto L_0x00597e5a;
  eax = s_030;
  fp1 = x86::Float(rc::ldf32(ebx));
  fp1 = psub(fp1, Float(rc::ldf32(eax - 0x8u)));
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax - 0x4u)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax)));
  s_238 = bits(fp3);
  ecx = s_238;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_050 = bits(fp2);
  s_058 = ecx;
  ecx = s_02c;
  s_054 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(ecx));
  fp1 = psub(fp1, Float(rc::ldf32(eax - 0x8u)));
  fp2 = x86::Float(rc::ldf32(ecx + 0x4u));
  fp2 = psub(fp2, Float(rc::ldf32(eax - 0x4u)));
  fp3 = x86::Float(rc::ldf32(ecx + 0x8u));
  fp3 = psub(fp3, Float(rc::ldf32(eax)));
  s_298 = bits(fp3);
  edx = s_298;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_0f8 = edx;
  s_0f0 = bits(fp2);
  s_0f4 = bits(fp1);
  fp1 = x86::Float(fl(s_0f0));
  fp1 = pmul(fp1, Float(fl(s_0b8)));
  fp2 = x86::Float(fl(s_0f8));
  fp2 = pmul(fp2, Float(fl(s_0b0)));
  fp1 = psub(fp1, fp2);
  fp2 = x86::Float(fl(s_0f4));
  fp2 = pmul(Float(fl(s_0b0)), fp2);
  fp3 = x86::Float(fl(s_0b4));
  fp3 = pmul(Float(fl(s_0f0)), fp3);
  fp2 = psub(fp2, fp3);
  fp3 = x86::Float(fl(s_0f8));
  fp3 = pmul(fp3, Float(fl(s_0b4)));
  fp4 = x86::Float(fl(s_0f4));
  fp4 = pmul(Float(fl(s_0b8)), fp4);
  fp3 = psub(fp3, fp4);
  s_100 = bits(fp3);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  s_104 = bits(fp2);
  s_108 = bits(fp1);
L_ret_597c85:
  fp1 = x86::Float(fl(s_108));
  fp1 = pmul(fp1, Float(fl(s_058)));
  fp2 = x86::Float(fl(s_104));
  fp2 = pmul(fp2, Float(fl(s_054)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_100));
  fp2 = pmul(fp2, Float(fl(s_050)));
  fp1 = padd(fp1, fp2);
  lzfa_597cba = fp1;
  lzfb_597cba = kZero;
  if ((x86::reg8((!(lzfa_597cba >= lzfb_597cba) ? 0x01 : 0) | (!__builtin_islessgreater(lzfa_597cba, lzfb_597cba) ? 0x40 : 0)) != 0)) goto L_0x00597e5a;
  fp1 = x86::Float(fl(s_024));
  ecx = rc::ld32(ebx);
  fp1 = psub(fp1, Float(fl(s_08c)));
  fp2 = fp1;
  fp2 = pmul(fp2, Float(rc::ldf32(esi)));
  fp3 = fp1;
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x4u)));
  {
  const x86::Float t = fp3;
  fp3 = fp1;
  fp1 = t;
  }
  fp3 = pmul(fp3, Float(rc::ldf32(esi + 0x8u)));
  s_218 = bits(fp3);
  s_250 = bits(fp2);
  eax = s_250;
  fp2 = x86::Float(fl(s_218));
  s_0a0 = eax;
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  eax = s_028;
  s_0a4 = bits(fp2);
  fp2 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp3 = x86::Float(rc::ldf32(ebx + 0x8u));
  rc::st32(edi + 0x80u, ecx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  ecx = s_048;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597d20 = a;
  lzb_597d20 = b;
  }
  rc::stf32(edi + 0x84u, float(fp3));
  rc::stf32(edi + 0x88u, float(fp2));
  fp2 = x86::Float(fl(s_0a0));
  fp2 = padd(fp2, Float(rc::ldf32(ebx)));
  fp3 = x86::Float(rc::ldf32(ebx + 0x4u));
  fp3 = padd(fp3, Float(fl(s_0a4)));
  fp4 = x86::Float(rc::ldf32(ebx + 0x8u));
  fp4 = padd(fp1, fp4);
  s_2a8 = bits(fp4);
  edx = s_2a8;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x90u, float(fp3));
  rc::stf32(edi + 0x94u, float(fp2));
  rc::st32(edi + 0x98u, edx);
  fp2 = fp1;
  fp1 = pmul(fp1, fp2);
  fp2 = x86::Float(fl(s_0a4));
  fp2 = pmul(fp2, Float(fl(s_0a4)));
  fp1 = padd(fp1, fp2);
  fp2 = x86::Float(fl(s_0a0));
  fp2 = pmul(fp2, Float(fl(s_0a0)));
  fp1 = padd(fp1, fp2);
  if ((x86::reg32(lza_597d20) == x86::reg32(lzb_597d20))) goto L_0x0059803c;
  lzfa_597d98 = fp1;
  lzfb_597d98 = x86::Float(fl(s_068));
  if (!(lzfa_597d98 < lzfb_597d98)) goto L_0x00597e58;
  edx = rc::ld32(edi + 0x80u);
  s_068 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x84u));
  s_160 = edx;
  edx = rc::ld32(edi + 0x90u);
  s_150 = edx;
  edx = rc::ld32(esi);
  s_164 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x88u));
  s_140 = edx;
  edx = rc::ld32(esi + 0x4u);
  s_168 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x94u));
  eax = 0x1u;
  s_144 = edx;
  edx = rc::ld32(esi + 0x8u);
  s_154 = bits(fp1);
  fp1 = x86::Float(rc::ldf32(edi + 0x98u));
  s_0cc = eax;
  s_148 = edx;
  edx = s_03c;
  s_13c = eax;
  eax = s_01c;
  IB[0] = ecx;
  ecx = s_044;
  s_158 = bits(fp1);
  IA[0] = eax;
  s_128 = 0x3u;
  IB[1] = ecx;
  IB[2] = edx;
  goto L_0x00597e5a;
L_0x00597e58:
  fp1 = x86::Float(fp1);
L_0x00597e5a:
  edx = s_030;
  ebx = s_088;
  eax = s_03c;
  ecx = 0x10u;
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 a = ebx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  ebx = r;
  }
  ecx = s_04c;
  s_030 = edx;
  edx = s_06c;
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  {
  const x86::reg32 r = x86::reg32(edx - 1);
  edx = r;
  }
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597e80 = a;
  lzb_597e80 = b;
  }
  s_03c = eax;
  s_088 = ebx;
  s_06c = edx;
  if ((x86::sreg32(x86::reg32(lza_597e80)) < x86::sreg32(x86::reg32(lzb_597e80)))) goto L_0x005976d0;
L_0x00597e97:
  ecx = s_044;
  edx = s_034;
  eax = s_038;
  ebx = s_020;
  {
  const x86::reg32 r = x86::reg32(ecx + 1);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  edx = r;
  }
  {
  const x86::reg32 r = x86::reg32(eax + 1);
  eax = r;
  }
  s_038 = eax;
  eax = s_04c;
  {
  const x86::reg32 r = x86::reg32(ebx - 1);
  ebx = r;
  }
  {
  const x86::reg32 r = x86::reg32(eax - 1);
  eax = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597eb6 = a;
  lzb_597eb6 = b;
  }
  s_044 = ecx;
  s_034 = edx;
  s_020 = ebx;
  if ((x86::sreg32(x86::reg32(lza_597eb6)) < x86::sreg32(x86::reg32(lzb_597eb6)))) goto L_0x00597683;
L_0x00597eca:
  ecx = s_02c;
  ebx = s_018;
  edx = s_064;
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  s_02c = ecx;
  ecx = s_04c;
  {
  const x86::reg32 r = x86::reg32(ebx - 1);
  ebx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0xfffffffeu;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = ecx;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597ee5 = a;
  lzb_597ee5 = b;
  }
  s_048 = edx;
  s_018 = ebx;
  if ((x86::sreg32(x86::reg32(lza_597ee5)) < x86::sreg32(x86::reg32(lzb_597ee5)))) goto L_0x00597650;
L_0x00597ef5:
  eax = s_0cc;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a & b);
  lzr_597efc = r;
  }
  if ((x86::reg32(lzr_597efc) == 0)) goto L_0x0059810a;
  eax = s_160;
  ecx = s_164;
  edx = s_168;
  rc::st32(edi + 0x80u, eax);
  eax = s_150;
  rc::st32(edi + 0x84u, ecx);
  ecx = s_154;
  rc::st32(edi + 0x88u, edx);
  edx = s_158;
  rc::st32(edi + 0x90u, eax);
  eax = s_140;
  rc::st32(edi + 0x94u, ecx);
  ecx = s_144;
  rc::st32(edi + 0x98u, edx);
  edx = s_148;
  rc::st32(esi, eax);
  eax = s_13c;
  rc::st32(esi + 0x4u, ecx);
  ecx = s_128;
  rc::st32(esi + 0x8u, edx);
  edx = 0;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a & b);
  lzr_597f7f = r;
  }
  rc::st32(edi + 0xc0u, eax);
  rc::st32(edi + 0xd4u, ecx);
  if ((x86::sreg32(x86::reg32(lzr_597f7f)) <= 0)) goto L_0x00597fdb;
  ecx = x86::reg32(edi + 0x8u);
  esi = x86::reg32(edi + 0xb0u);
  goto L_0x00597fa0;
L_0x00597fa0:
  eax = IA[edx];
  ebx = rc::ld32(edi + eax * 4 + 0xb0u);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  rc::st32(esi, ebx);
  fp1 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  eax = rc::ld32(eax);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::st32(ecx - 0x8u, eax);
  rc::stf32(ecx - 0x4u, float(fp2));
  eax = s_13c;
  rc::stf32(ecx, float(fp1));
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = esi;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  esi = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_597fd7 = a;
  lzb_597fd7 = b;
  }
  if ((x86::sreg32(x86::reg32(lza_597fd7)) < x86::sreg32(x86::reg32(lzb_597fd7)))) goto L_0x00597fa0;
L_0x00597fdb:
  eax = s_128;
  edx = 0;
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a & b);
  lzr_597fe4 = r;
  }
  if ((x86::sreg32(x86::reg32(lzr_597fe4)) <= 0)) goto L_0x0059802f;
  esi = x86::reg32(edi + 0xc4u);
  ecx = x86::reg32(edi + 0x48u);
L_0x00597ff1:
  eax = IB[edx];
  ebx = rc::ld32(edi + eax * 4 + 0xc4u);
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  rc::st32(esi, ebx);
  fp1 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  eax = rc::ld32(eax);
  {
  const x86::Float t = fp2;
  fp2 = fp1;
  fp1 = t;
  }
  rc::st32(ecx - 0x8u, eax);
  rc::stf32(ecx - 0x4u, float(fp2));
  eax = s_128;
  rc::stf32(ecx, float(fp1));
  {
  const x86::reg32 r = x86::reg32(edx + 1);
  edx = r;
  }
  {
  const x86::reg32 a = ecx;
  const x86::reg32 b = 0x10u;
  const x86::reg32 r = x86::reg32(a + b);
  ecx = r;
  }
  {
  const x86::reg32 a = esi;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  esi = r;
  }
  {
  const x86::reg32 a = edx;
  const x86::reg32 b = eax;
  const x86::reg32 r = x86::reg32(a - b);
  lza_59802b = a;
  lzb_59802b = b;
  }
  if ((x86::sreg32(x86::reg32(lza_59802b)) < x86::sreg32(x86::reg32(lzb_59802b)))) goto L_0x00597ff1;
L_0x0059802f:
  fp1 = x86::Float(fl(s_068));
  finish(cpu, fpu_count, fp1);
  return;
L_0x0059803c:
  eax = s_01c;
  rc::st32(edi + 0xc0u, 0x1u);
  edx = rc::ld32(edi + eax * 4 + 0xb0u);
  rc::st32(edi + 0xb0u, edx);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  fp3 = x86::Float(rc::ldf32(eax + 0x8u));
  eax = rc::ld32(eax);
  rc::st32(edi, eax);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x4u, float(fp3));
  eax = x86::reg32(ecx + 0x4u);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  rc::stf32(edi + 0x8u, float(fp2));
  rc::st32(edi + 0xd4u, 0x3u);
  edx = rc::ld32(edi + ecx * 4 + 0xc4u);
  rc::st32(edi + 0xc4u, edx);
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  eax = rc::ld32(eax);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x40u, eax);
  rc::stf32(edi + 0x44u, float(fp3));
  eax = s_044;
  rc::stf32(edi + 0x48u, float(fp2));
  ecx = rc::ld32(edi + eax * 4 + 0xc4u);
  rc::st32(edi + 0xc8u, ecx);
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  edx = rc::ld32(eax + edi);
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  eax = s_03c;
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::st32(edi + 0x50u, edx);
  rc::stf32(edi + 0x54u, float(fp3));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = 0x4u;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  rc::stf32(edi + 0x58u, float(fp2));
  ecx = rc::ld32(edi + eax * 4 + 0xb4u);
  rc::st32(edi + 0xccu, ecx);
  {
  const x86::reg32 a = eax;
  const unsigned n = 4;
  const x86::reg32 r = x86::reg32(x86::reg32(a) << n);
  eax = r;
  }
  fp2 = x86::Float(rc::ldf32(eax + edi + 0x4u));
  edx = rc::ld32(eax + edi);
  fp3 = x86::Float(rc::ldf32(eax + edi + 0x8u));
  {
  const x86::reg32 a = eax;
  const x86::reg32 b = edi;
  const x86::reg32 r = x86::reg32(a + b);
  eax = r;
  }
  rc::st32(edi + 0x60u, edx);
  {
  const x86::Float t = fp3;
  fp3 = fp2;
  fp2 = t;
  }
  rc::stf32(edi + 0x64u, float(fp3));
  rc::stf32(edi + 0x68u, float(fp2));
  finish(cpu, fpu_count, fp1);
  return;
L_0x0059810a:
  fp1 = kZero;
  finish(cpu, fpu_count, fp1);
  return;
}

void body(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!pf::cachedStatic([] { return pf::Config::getBool("closest_feature_native", true); }))
    {
        ug2::sub_595040_guest(app, cpu);
        return;
    }
    search(app, cpu);
}

void declare(x86::CPU& cpu, shadow::Spans& spans)
{
    spans.push_back({cpu.ecx, 0xd8, false});
    spans.push_back({argument(cpu, 0), 12, false});
}

const shadow::Spec spec = {"sub_595040", shadow::Ebx | shadow::Ebp | shadow::Esi | shadow::Edi, 4, false, false};

}

void sub_595040(win32::WinApplication* app, x86::CPU& cpu)
{
    shadow::call(spec, app, cpu, body, sub_595040_guest, declare);
}

}}
