cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(SceDs3Ext)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O2 -fno-inline")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

link_directories(
  # ${CMAKE_SOURCE_DIR}/extra/
)

add_executable(SceDs3Ext
  src/ds3ext.c
)

target_link_libraries(SceDs3Ext
  SceCpuForDriver_stub
  SceDebugForDriver_stub
  SceModulemgrForDriver_stub
  SceSblAIMgrForDriver_stub
  SceSysclibForDriver_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
)

set_target_properties(SceDs3Ext
  PROPERTIES LINK_FLAGS "-nostdlib"
  COMPILE_FLAGS "-D__PSP2_KERNEL__ -DDS3EXT_DEBUG_X"
)

vita_create_self(ds3ext.skprx SceDs3Ext
  CONFIG exports.yml
  UNSAFE
  STRIPPED
  REL_OPTIMIZE
)
