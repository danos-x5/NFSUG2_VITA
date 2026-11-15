# Writes <GXP>/sources.txt, the record of one compile that cmake/shaders.cmake checks. Run it straight after
# tools/shadercomp/vita_compile.py --out <GXP>:  cmake -DGXP=<GXP> -P cmake/shader_sources.cmake
include(${CMAKE_CURRENT_LIST_DIR}/shaders.cmake)

if(NOT GXP OR NOT IS_DIRECTORY "${GXP}")
  message(FATAL_ERROR "usage: cmake -DGXP=<folder of compiled .gxp files> -P cmake/shader_sources.cmake")
endif()
set_property(GLOBAL PROPERTY NFSU_SHADER_RECORDS "")

function(nfsu_add_shader out_var name profile)
  cmake_parse_arguments(SHADER "PENDING" "SOURCE" "DEFINES" ${ARGN})
  if(NOT EXISTS "${GXP}/${name}.gxp")
    if(SHADER_PENDING)
      return()
    endif()
    message(FATAL_ERROR "${GXP}/${name}.gxp is missing: the compile did not finish")
  endif()
  nfsu_shader_record(record ${name} "${GXP}/${name}.gxp" "${SHADER_SOURCE}" "${SHADER_DEFINES}")
  set_property(GLOBAL APPEND_STRING PROPERTY NFSU_SHADER_RECORDS "${record}\n")
endfunction()

nfsu_add_shaders(programs)
get_property(records GLOBAL PROPERTY NFSU_SHADER_RECORDS)
file(WRITE "${GXP}/sources.txt"
     "# <program> <SHA-256 of the .gxp> <SHA-256 of its Cg source> <source> [defines]: cmake/shader_sources.cmake\n"
     "${records}")
string(REGEX MATCHALL "\n" lines "${records}")
list(LENGTH lines count)
message("${GXP}/sources.txt: ${count} programs")
