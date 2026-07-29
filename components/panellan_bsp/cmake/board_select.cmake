# SPDX-License-Identifier: Apache-2.0
#
# Board-aware SDKCONFIG_DEFAULTS for PanelLan BSP.
#
# MUST be included from the *project* root CMakeLists.txt BEFORE
#   include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# A component's own CMakeLists.txt runs too late — SDKCONFIG_DEFAULTS is
# already consumed by then.
#
# New project (after copying components/panellan_bsp):
#
#   include(${CMAKE_CURRENT_LIST_DIR}/components/panellan_bsp/cmake/board_select.cmake)
#
# Then: idf.py -DBOARD=<target dir name> set-target esp32s3 build

if(NOT DEFINED BOARD OR BOARD STREQUAL "")
    return()
endif()

set(BOARD "${BOARD}" CACHE STRING "PanelLan board under panellan_bsp/target/" FORCE)

# This file lives at: <proj>/components/panellan_bsp/cmake/board_select.cmake
get_filename_component(_panellan_bsp_comp "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(_panellan_proj_root "${_panellan_bsp_comp}/../.." ABSOLUTE)

set(_panellan_defaults_list "sdkconfig.defaults")

if(DEFINED IDF_TARGET AND NOT IDF_TARGET STREQUAL "")
    set(_chip_defaults "sdkconfig.defaults.${IDF_TARGET}")
else()
    set(_chip_defaults "sdkconfig.defaults.esp32s3")
endif()
if(EXISTS "${_panellan_proj_root}/${_chip_defaults}")
    string(APPEND _panellan_defaults_list ";${_chip_defaults}")
endif()

set(_board_defaults "${_panellan_bsp_comp}/target/${BOARD}/sdkconfig.defaults")
if(NOT EXISTS "${_board_defaults}")
    message(FATAL_ERROR
        "Board defaults not found: ${_board_defaults} (check -DBOARD=)")
endif()
# SDKCONFIG_DEFAULTS paths are resolved relative to the project directory
file(RELATIVE_PATH _board_defaults_from_proj "${_panellan_proj_root}" "${_board_defaults}")
string(APPEND _panellan_defaults_list ";${_board_defaults_from_proj}")

set(SDKCONFIG_DEFAULTS "${_panellan_defaults_list}" CACHE STRING
    "Board-aware sdkconfig defaults" FORCE)
message(STATUS "PanelLan board: ${BOARD}")
message(STATUS "SDKCONFIG_DEFAULTS: ${SDKCONFIG_DEFAULTS}")
