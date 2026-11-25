// NFSU/NFSU2 Visual Reputation Remover 1.1's NOP run at 0x557EDE: the reputation meter's message call, where MOD_SWITCHES redirects it.

#include <native/common.h>

namespace ug2 { namespace native {

// Pops the five arguments of 0x555D00 (stdcall, ret 0x14) and changes nothing else.
void visualRepMeter(win32::WinApplication*, x86::CPU& cpu)
{
    // NFSU/NFSU2 Visual Reputation Remover 1.1 [NFSU2] RemoveVisualRep: 0x557EDE NOPs push 0 x3, the .fng name, push 0 and the call
    cpu.esp += 0x18;
}

}}
