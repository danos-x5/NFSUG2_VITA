# Third-party code and licensing

This repository's own code is licensed under the MIT License ([LICENSE](LICENSE)), which covers the port's own
source. The parts below come from other projects and keep their own terms.

## UG1VITA

This project grew out of [UG1VITA](https://github.com/NDRWhun/UG1VITA) by NDRWhun, the PS Vita port of Need for
Speed Underground, and still contains code from it. UG1VITA is MIT licensed; its copyright notice is kept in
[LICENSE](LICENSE) beside this project's own, as that license requires.

## pefile

`tools/recompiler/disasm/pefile.py` and `tools/recompiler/disasm/ordlookup/`, from the pefile project (Ero Carrera,
version 2018.8.8), MIT License. The license text is in `tools/recompiler/disasm/LICENSE.pefile`.

## Capstone

The recompiler disassembles with Capstone (BSD 3-Clause License), version pinned in
`tools/recompiler/requirements.txt`. It is not part of the repository: pip or `tools/recompiler/bootstrap.py`
installs it into `.pydeps/`.

## LLVM compiler-rt

The optional profile-guided build links compiler-rt's profile runtime (Apache License 2.0 with LLVM Exceptions). It
is not part of the repository: `cmake/pgo-runtime.cmake` downloads it for the compiler's version.

## Need for Speed Underground 2

Game code, binaries and assets are the property of Electronic Arts Inc. and its licensors and are not part of this
repository. Users supply their own copy of the game; the code generated from its executable (`src/ug2/generated/`)
is not tracked.
