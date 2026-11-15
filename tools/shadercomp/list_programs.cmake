# Prints the programs cmake/shaders.cmake builds, one manifest line each: <name> <profile> <source> [DEFINES...]
#   cmake -DREPO=<repo> -P tools/shadercomp/list_programs.cmake
include(${REPO}/cmake/shaders.cmake)

function(nfsu_add_shader out_var name profile)
  cmake_parse_arguments(SHADER "PENDING" "SOURCE" "DEFINES" ${ARGN})
  if(NOT SHADER_SOURCE)
    set(SHADER_SOURCE "${name}.cg")
  endif()
  string(REPLACE ";" " " defines "${SHADER_DEFINES}")
  message("${name} ${profile} ${SHADER_SOURCE} ${defines}")
endfunction()

nfsu_add_shaders(programs)
