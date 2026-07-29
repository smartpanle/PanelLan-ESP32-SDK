# ESP-IDF project root CMakeLists — keep this order or the build breaks.
cmake_minimum_required(VERSION 3.16)

# PanelLan board select (logic lives in the component; must run before project.cmake):
#   idf.py -DBOARD=<target dir under panellan_bsp/target/> set-target esp32s3 build
include(${CMAKE_CURRENT_LIST_DIR}/components/panellan_bsp/cmake/board_select.cmake)

add_definitions(-DLV_MEM_CUSTOM=1)
add_definitions(-DCONFIG_LV_MEM_POOL_ALLOC=malloc)
add_definitions(-DCONFIG_LV_MEM_POOL_INCLUDE=<stdlib.h>)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Minimal build: only main and its dependencies
idf_build_set_property(MINIMAL_BUILD ON)
project(hello_world)
