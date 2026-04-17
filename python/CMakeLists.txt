cmake_minimum_required(VERSION 3.15)
project(teslasynth)

find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

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
)

target_include_directories(_teslasynth PRIVATE
    ${LIB_DIR}
    ${LIB_DIR}/synthesizer
)

target_compile_features(_teslasynth PRIVATE cxx_std_23)

install(TARGETS _teslasynth LIBRARY DESTINATION teslasynth)
