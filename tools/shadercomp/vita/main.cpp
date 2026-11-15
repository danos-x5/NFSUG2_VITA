// NFSU2VITA before the game is built: only the shader compile mode (tools/shadercomp).

#include "../shadercomp.h"
#include <psp2/kernel/processmgr.h>

extern "C" {
int _newlib_heap_size_user = 96 * 1024 * 1024;
}

int main()
{
    shadercomp::runIfRequested("ux0:data/NeedForSpeedUnderground2/shaders");
    sceKernelExitProcess(0);
    return 0;
}
