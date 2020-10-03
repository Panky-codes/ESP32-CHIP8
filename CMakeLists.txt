# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS externals/ESP32_TFT_library/components)
set(CMAKE_CXX_STANDARD 17)

project(CHIP8)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

