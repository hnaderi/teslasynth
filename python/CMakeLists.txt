# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: LGPL-3.0-only

cmake_minimum_required(VERSION 3.15)
project(teslasynth)

find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

find_package(Git QUIET)
if(Git_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE TESLASYNTH_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
endif()
if(NOT TESLASYNTH_VERSION OR TESLASYNTH_VERSION STREQUAL "")
  set(TESLASYNTH_VERSION "unknown")
endif()

# Ask the active Python where nanobind installed its cmake files.
# This works regardless of whether nanobind is in a venv, nix store, or site-packages.
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE NB_CMAKE_DIR
)
find_package(nanobind CONFIG REQUIRED HINTS ${NB_CMAKE_DIR})

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib")

nanobind_add_module(
    _teslasynth
    src/bindings.cpp
    ${LIB_DIR}/synthesizer/curve.cpp
    ${LIB_DIR}/synthesizer/lfo.cpp
    ${LIB_DIR}/synthesizer/voice_event.cpp
    ${LIB_DIR}/synthesizer/voices/note.cpp
    ${LIB_DIR}/synthesizer/voices/hit.cpp
    ${LIB_DIR}/teslasynth/config_parser.cpp
    ${LIB_DIR}/teslasynth/config_patch_update.cpp
)

target_include_directories(_teslasynth PRIVATE
    ${LIB_DIR}
    ${LIB_DIR}/synthesizer
    ${LIB_DIR}/teslasynth
)

target_compile_features(_teslasynth PRIVATE cxx_std_20)
target_compile_definitions(_teslasynth PRIVATE
    TESLASYNTH_VERSION="${TESLASYNTH_VERSION}"
    TESLASYNTH_BUILD_DATE=__DATE__
    TESLASYNTH_BUILD_TIME=__TIME__
)

install(TARGETS _teslasynth LIBRARY DESTINATION teslasynth)
