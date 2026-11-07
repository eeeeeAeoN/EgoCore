cmake_minimum_required(VERSION 3.21)

project(fable-defs-workspace LANGUAGES CXX)

include(CTest)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# The C++ projects live under cpp/. This root project exists so a single
# `cmake --preset` from the repository root builds the model and the compiler
# together.
add_subdirectory(cpp)
