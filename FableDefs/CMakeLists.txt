cmake_minimum_required(VERSION 3.21)

project(FableDefs LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(FableDefs STATIC
    src/build.cpp
    src/build_index.cpp
    src/build_pipeline.cpp
    src/definitions.cpp
    src/diagnostic.cpp
    src/lexer.cpp
    src/lower.cpp
    src/names.cpp
    src/parser.cpp
    src/symbols.cpp
    src/wire.cpp)

target_include_directories(FableDefs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(FableDefs PUBLIC cxx_std_20)

add_executable(fable-defc tools/defc_main.cpp)
target_link_libraries(fable-defc PRIVATE FableDefs)

if(MSVC)
    target_compile_options(FableDefs PRIVATE /W4)
    target_compile_options(fable-defc PRIVATE /W4)
else()
    target_compile_options(FableDefs PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(fable-defc PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    function(fable_defs_test name source)
        add_executable(${name} tests/${source})
        target_link_libraries(${name} PRIVATE FableDefs)
        if(MSVC)
            target_compile_options(${name} PRIVATE /W4 /WX)
        else()
            target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    fable_defs_test(fable-defs-test defs_test.cpp)
    fable_defs_test(fable-defs-compiler-test parser_test.cpp)
    fable_defs_test(fable-defs-lower-test lower_test.cpp)
    fable_defs_test(fable-defs-wire-test wire_test.cpp)
    fable_defs_test(fable-defs-corpus-test corpus_test.cpp)
    fable_defs_test(fable-defs-build-test build_test.cpp)
endif()
