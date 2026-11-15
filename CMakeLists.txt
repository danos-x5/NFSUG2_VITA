# NFSU2VITA - Need for Speed Underground 2 (static recompilation) -> PS Vita. ELF -> SELF -> VPK.
cmake_minimum_required(VERSION 3.16)

# The PS Vita build (the VPK). The macOS test build is on the development branch.

option(NFSU_RUNTIME_CHECK "Only compile the runtime sources that build without the generated code (CI)" OFF)
# gcc: VitaSDK's compiler, PGO (cmake/gcc.cmake). clang: clang and lld (cmake/vita-clang.cmake, LLVM from NFSU_LLVM),
# ThinLTO, PGO.
set(NFSU_COMPILER "gcc" CACHE STRING "Compiler: gcc (VitaSDK) or clang (LLVM)")
set_property(CACHE NFSU_COMPILER PROPERTY STRINGS gcc clang)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(NFSU_COMPILER STREQUAL "clang")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/vita-clang.cmake" CACHE PATH "toolchain file")
  elseif(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Define the VITASDK environment variable (point at your VitaSDK root).")
  endif()
endif()

project(NFSU2VITA C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# GNU mode keeps newlib's POSIX declarations (ftruncate, strcasecmp) visible.
set(CMAKE_CXX_EXTENSIONS ON)

include("${VITASDK}/share/vita.cmake" REQUIRED)

if(NFSU_COMPILER STREQUAL "clang")
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "NFSU_COMPILER=clang needs the toolchain file cmake/vita-clang.cmake")
  endif()
  set(NFSU_CLANG ON)
elseif(NFSU_COMPILER STREQUAL "gcc")
  set(NFSU_CLANG OFF)
else()
  message(FATAL_ERROR "NFSU_COMPILER must be gcc or clang")
endif()

set(VITA_APP_NAME "NFS Underground 2")
set(VITA_TITLEID  "NFSU2VITA")
set(NFSU_VERSION  "0.8.0")
set(VITA_VERSION  "00.80")

option(NFSU_LOG_DEBUG        "Verbose runtime logging" OFF)
option(NFSU_ASSERT_SOFT      "Log failed assertions instead of trapping" OFF)
# Profile-guided optimisation: cmake/gcc.cmake, cmake/clang.cmake.
option(NFSU_PGO_GENERATE "Instrument for a PGO profile" OFF)
set(NFSU_PGO_USE "" CACHE STRING "Profile to optimise with: nfsu.gcov files (gcc, a list) or an indexed .profdata (clang)")
option(NFSU_PGO_DEFAULT "clang: optimise with pgo/nfsu2.proftext when NFSU_PGO_USE is empty" ON)
if(NFSU_PGO_GENERATE AND NFSU_PGO_USE)
  message(FATAL_ERROR "NFSU_PGO_GENERATE and NFSU_PGO_USE exclude each other")
endif()
# The profile names file-local functions by source path, so it is stored with @NFSU_SOURCE_DIR@ in its place.
if(NFSU_CLANG AND NFSU_PGO_DEFAULT AND NOT NFSU_PGO_GENERATE AND NOT NFSU_PGO_USE
   AND EXISTS "${CMAKE_SOURCE_DIR}/pgo/nfsu2.proftext")
  file(READ "${CMAKE_SOURCE_DIR}/pgo/nfsu2.proftext" NFSU_PGO_TEXT)
  string(REPLACE "@NFSU_SOURCE_DIR@" "${CMAKE_SOURCE_DIR}" NFSU_PGO_TEXT "${NFSU_PGO_TEXT}")
  file(WRITE "${CMAKE_BINARY_DIR}/pgo/nfsu2.proftext" "${NFSU_PGO_TEXT}")
  unset(NFSU_PGO_TEXT)
  execute_process(COMMAND "${NFSU_LLVM_PROFDATA}" merge -o "${CMAKE_BINARY_DIR}/pgo/nfsu2.profdata"
                          "${CMAKE_BINARY_DIR}/pgo/nfsu2.proftext" RESULT_VARIABLE NFSU_PGO_MERGE)
  if(NFSU_PGO_MERGE)
    message(FATAL_ERROR "llvm-profdata could not index pgo/nfsu2.proftext (${NFSU_PGO_MERGE})")
  endif()
  set(NFSU_PGO_USE "${CMAKE_BINARY_DIR}/pgo/nfsu2.profdata")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/pgo/nfsu2.proftext")
endif()

include(cmake/shaders.cmake)
include(cmake/sources.cmake)

nfsu_add_shaders(NFSU_SHADER_SOURCES)

if(NFSU_CLANG)
  set(NFSU_ARCH_FLAGS -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard)
  # Every multiply and add rounded on its own, as in the gcc build (the VFPv3 unit has no fused multiply-add).
  set(NFSU_FP_FLAGS -ffp-contract=off)
else()
  set(NFSU_ARCH_FLAGS -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard)
  set(NFSU_FP_FLAGS "")
endif()
set(NFSU_COMMON_FLAGS
  ${NFSU_ARCH_FLAGS}
  ${NFSU_FP_FLAGS}
  # Math functions do not set errno: sqrt is a single vsqrt.f64.
  -fno-math-errno
  -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables
  -ffunction-sections -fdata-sections
  -Wall -Wextra
  # __FILE__ in assertions and logs: paths relative to the repository, nothing from the build machine.
  -ffile-prefix-map=${CMAKE_SOURCE_DIR}/=
)
# Generated code only sees the stable defines; debug switches stay in the runtime to avoid full rebuilds.
option(NFSU_FPU_DOUBLE "x87 values in double (the default is float, as the game runs the x87 at 24-bit precision)" OFF)
if(NFSU_FPU_DOUBLE)
  add_compile_definitions(NFSU_FPU_DOUBLE=1)
endif()
set(NFSU_DEFINES VITA __vita__ WITH_MMX)
set(NFSU_RUNTIME_DEFINES ${NFSU_DEFINES} NFSU_VERSION="${NFSU_VERSION}")
if(NFSU_LOG_DEBUG)
  list(APPEND NFSU_RUNTIME_DEFINES NFSU_LOG_DEBUG)
endif()
if(NFSU_ASSERT_SOFT)
  list(APPEND NFSU_RUNTIME_DEFINES NFSU_ASSERT_SOFT)
endif()
# Shadow compare of native functions (src/ug2/native/shadow.h): development builds only, it adds a check per D3D call.
option(NFSU_NATIVE_SHADOW "Shadow compare of native functions (native_shadow=on)" OFF)
if(NFSU_NATIVE_SHADOW)
  list(APPEND NFSU_RUNTIME_DEFINES NFSU_NATIVE_SHADOW)
endif()

set(NFSU_GENERATED_DIR "${CMAKE_SOURCE_DIR}/src/ug2/generated" CACHE PATH "Generated game code (tools/recompiler)")
set(NFSU_INCLUDES
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/ug2
  ${NFSU_GENERATED_DIR}
)

# Compiles the runtime sources that do not need the generated game code; no VPK (CI without game binaries).
if(NFSU_RUNTIME_CHECK)
  add_library(nfsu_runtime_check OBJECT ${NFSU_STANDALONE_SOURCES} ${NFSU_SHADER_SOURCES})
  target_compile_options(nfsu_runtime_check PRIVATE ${NFSU_COMMON_FLAGS} -O3 -Werror)
  target_compile_definitions(nfsu_runtime_check PRIVATE ${NFSU_RUNTIME_DEFINES})
  target_include_directories(nfsu_runtime_check PRIVATE ${NFSU_INCLUDES})
  return()
endif()

file(GLOB NFSU_GENERATED_SOURCES CONFIGURE_DEPENDS "${NFSU_GENERATED_DIR}/*.cpp")
if(NOT NFSU_GENERATED_SOURCES)
  message(FATAL_ERROR "${NFSU_GENERATED_DIR} is empty; run tools/ug2-generate.ps1 (or step 1 of the README) first.")
endif()

if(NFSU_CLANG)
  include(cmake/clang.cmake)
  set(NFSU_NO_SLP -fno-slp-vectorize)
else()
  include(cmake/gcc.cmake)
  set(NFSU_NO_SLP -fno-tree-slp-vectorize)
endif()

# Generated game code: large, optimise for size and compile time.
add_library(nfsu_game OBJECT ${NFSU_GENERATED_SOURCES})
# No SLP vectorisation: it packs guest stores into NEON stores through the stack (store forwarding stalls).
set(NFSU_GAME_OPT "-O2" CACHE STRING "Optimisation level of the generated code")
target_compile_options(nfsu_game PRIVATE ${NFSU_COMMON_FLAGS} ${NFSU_GAME_OPT} ${NFSU_NO_SLP} -Wno-unused-label -Wno-unused-variable -Wno-unused-parameter ${NFSU_CLANG_GAME_FLAGS} ${NFSU_GCC_GAME_FLAGS})
# Start-up method registration (nfsu.cpp, runs once) gains nothing from -O2 and compiles much faster at -Og.
set(NFSU_LIGHT_SOURCES ${NFSU_GENERATED_SOURCES})
list(FILTER NFSU_LIGHT_SOURCES INCLUDE REGEX "/ug2\\.cpp$")
set_source_files_properties(${NFSU_LIGHT_SOURCES} PROPERTIES COMPILE_OPTIONS -Og)
if(NFSU_CLANG)
  set_property(SOURCE ${NFSU_LIGHT_SOURCES} APPEND PROPERTY COMPILE_OPTIONS ${NFSU_CLANG_LIGHT_FLAGS})
endif()
target_compile_definitions(nfsu_game PRIVATE ${NFSU_DEFINES})
target_include_directories(nfsu_game PRIVATE ${NFSU_INCLUDES})

# Runtime and Vita backend.
add_library(nfsu_runtime OBJECT ${NFSU_RUNTIME_SOURCES} ${NFSU_SHADER_SOURCES})
target_compile_options(nfsu_runtime PRIVATE ${NFSU_COMMON_FLAGS} -O3 ${NFSU_CLANG_FLAGS} ${NFSU_GCC_FLAGS})
target_compile_definitions(nfsu_runtime PRIVATE ${NFSU_RUNTIME_DEFINES})
target_include_directories(nfsu_runtime PRIVATE ${NFSU_INCLUDES})
# The run-time helpers the code generator calls stay out of link-time optimisation, which would drop them.
set_source_files_properties(src/platform/memfuncs.cpp PROPERTIES COMPILE_OPTIONS -fno-lto)

add_executable(NFSU2VITA.elf src/ug2/main.cpp tools/shadercomp/shadercomp.cpp ${NFSU_CLANG_SOURCES} ${NFSU_GCC_SOURCES}
               $<TARGET_OBJECTS:nfsu_game> $<TARGET_OBJECTS:nfsu_runtime>)
target_compile_options(NFSU2VITA.elf PRIVATE ${NFSU_COMMON_FLAGS} -O2 ${NFSU_CLANG_FLAGS} ${NFSU_GCC_FLAGS})
target_compile_definitions(NFSU2VITA.elf PRIVATE ${NFSU_RUNTIME_DEFINES})
target_include_directories(NFSU2VITA.elf PRIVATE ${NFSU_INCLUDES})
# Nothing references the main thread's stack size; keep it from being collected so the loader sees it.
target_link_options(NFSU2VITA.elf PRIVATE -Wl,--gc-sections -Wl,--undefined=sceUserMainThreadStackSize)
# memcpy, memmove and memset go to the small-copy versions in src/platform/memfuncs.cpp.
target_link_options(NFSU2VITA.elf PRIVATE -Wl,--wrap=memcpy,--wrap=memmove,--wrap=memset)
set(NFSU_EXTRA_LINK_FLAGS "" CACHE STRING "Extra linker flags")
if(NFSU_EXTRA_LINK_FLAGS)
  target_link_options(NFSU2VITA.elf PRIVATE ${NFSU_EXTRA_LINK_FLAGS})
endif()
if(NFSU_CLANG)
  target_link_options(NFSU2VITA.elf PRIVATE ${NFSU_CLANG_LINK_FLAGS})
  target_link_libraries(NFSU2VITA.elf ${NFSU_CLANG_LIBRARIES})
  set_target_properties(NFSU2VITA.elf PROPERTIES LINK_DEPENDS "${NFSU_LLD_SCRIPT}")
else()
  target_link_options(NFSU2VITA.elf PRIVATE ${NFSU_GCC_LINK_FLAGS})
endif()
target_link_libraries(NFSU2VITA.elf
  stdc++
  SceShaccCg_stub_weak
  SceKernelModulemgr_stub
  SceIofilemgr_stub
  SceGxm_stub
  SceDisplay_stub
  SceCtrl_stub
  SceTouch_stub
  SceCommonDialog_stub
  SceAppUtil_stub
  SceAudio_stub
  SceVideodec_stub
  SceSysmodule_stub
  ScePower_stub
  SceRtc_stub
  SceLibKernel_stub
  SceKernelThreadMgr_stub
  SceSysmem_stub
  SceProcessmgr_stub
  c
  m
)

vita_create_self(NFSU2VITA.self NFSU2VITA.elf UNSAFE)

# ATTRIBUTE2=12: extended memory budget for the guest arena.
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")
configure_file(
  ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml.in
  ${CMAKE_BINARY_DIR}/generated/livearea/template.xml
  @ONLY)

vita_create_vpk(NFSU2VITA.vpk ${VITA_TITLEID} NFSU2VITA.self
  VERSION ${VITA_VERSION}
  NAME "${VITA_APP_NAME}"
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE ${CMAKE_BINARY_DIR}/generated/livearea/template.xml sce_sys/livearea/contents/template.xml
)
