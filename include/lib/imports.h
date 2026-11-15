#ifndef LIB_IMPORTS_H_
#define LIB_IMPORTS_H_

#include <lib/winapp.h>

namespace win32
{

// One import of SPEED2.EXE and the shim the generated registration binds to it.
struct ImportEntry
{
    const char* dll;    // lower case without .dll
    const char* name;
    MethodPtr   method;
};

// The import table of SPEED2.EXE, in file order.
const ImportEntry* importsBegin();
const ImportEntry* importsEnd();

}

#endif
