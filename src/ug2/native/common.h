#ifndef UG2_NATIVE_COMMON_H_
#define UG2_NATIVE_COMMON_H_

// Helpers shared by the hand-written native functions (src/ug2/native/sub_*.cpp, native.cpp).

#include <lib/guest.h>
#include <lib/winapp.h>

#include <cstring>
#include <fpu.h>
#include <native.h>
#include <recomp.h>

namespace ug2 { namespace native {

// Stack argument n of a cdecl or stdcall function.
inline x86::reg32 argument(x86::CPU& cpu, int n)
{
    return rc::ld32(cpu.esp + 4 + 4 * n);
}

using Real = x86::Float;

inline Real ld(x86::reg32 address) { return Real(rc::ldf32(address)); }

inline x86::reg32 bits(float value)
{
    x86::reg32 r;
    memcpy(&r, &value, 4);
    return r;
}

}}

#endif /* !UG2_NATIVE_COMMON_H_ */
