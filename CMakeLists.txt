# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.16.0)
set(EXTRA_COMPONENT_DIRS lib)
set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(teslasynth)
