#ifndef SHADERCOMP_H_
#define SHADERCOMP_H_

// Shader compile mode: <folder>/compile.txt lists Cg programs in <folder>/src; each is compiled with the system's
// PSP2 Cg compiler (libshacccg.suprx) into <folder>/out/<name>.gxp, with <folder>/compile.log as the report.

namespace shadercomp
{

// False when <folder>/compile.txt does not exist; true when the request ran (it is renamed to compile.done).
bool runIfRequested(const char* folder);

}

#endif
