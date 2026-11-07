include(GNUInstallDirs)

add_library(fable-defs-compiler
    src/diagnostic.cpp
    src/lexer.cpp
    src/parser.cpp
    src/symbols.cpp
    src/definitions.cpp
    src/build_index.cpp
    src/build_pipeline.cpp
    src/build.cpp
    src/lower.cpp
    src/wire.cpp
    src/names.cpp)

target_include_directories(fable-defs-compiler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(fable-defs-compiler PUBLIC fable::defs)
target_compile_features(fable-defs-compiler PUBLIC cxx_std_17)
set_target_properties(fable-defs-compiler PROPERTIES EXPORT_NAME defs-compiler)

add_executable(fable-defc src/defc_main.cpp)
target_link_libraries(fable-defc PRIVATE fable-defs-compiler)

if(MSVC)
    target_compile_options(fable-defs-compiler PRIVATE /W4)
    target_compile_options(fable-defc PRIVATE /W4)
else()
    target_compile_options(fable-defs-compiler PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(fable-defc PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(fable-defs-compiler-test tests/parser_test.cpp)
    target_link_libraries(fable-defs-compiler-test PRIVATE fable-defs-compiler)
    if(MSVC)
        target_compile_options(fable-defs-compiler-test PRIVATE /W4 /WX)
    else()
        target_compile_options(fable-defs-compiler-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME fable-defs-compiler-test COMMAND fable-defs-compiler-test)

    add_executable(fable-defs-lower-test tests/lower_test.cpp)
    target_link_libraries(fable-defs-lower-test PRIVATE fable-defs-compiler)
    if(MSVC)
        target_compile_options(fable-defs-lower-test PRIVATE /W4 /WX)
    else()
        target_compile_options(fable-defs-lower-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME fable-defs-lower-test COMMAND fable-defs-lower-test)

    add_executable(fable-defs-wire-test tests/wire_test.cpp)
    target_link_libraries(fable-defs-wire-test PRIVATE fable-defs-compiler)
    if(MSVC)
        target_compile_options(fable-defs-wire-test PRIVATE /W4 /WX)
    else()
        target_compile_options(fable-defs-wire-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME fable-defs-wire-test COMMAND fable-defs-wire-test)

    add_executable(fable-defs-corpus-test tests/corpus_test.cpp)
    target_link_libraries(fable-defs-corpus-test PRIVATE fable-defs-compiler)
    if(MSVC)
        target_compile_options(fable-defs-corpus-test PRIVATE /W4 /WX)
    else()
        target_compile_options(fable-defs-corpus-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME fable-defs-corpus-test COMMAND fable-defs-corpus-test)

    add_executable(fable-defs-build-test tests/build_test.cpp)
    target_link_libraries(fable-defs-build-test PRIVATE fable-defs-compiler)
    if(MSVC)
        target_compile_options(fable-defs-build-test PRIVATE /W4 /WX)
    else()
        target_compile_options(fable-defs-build-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME fable-defs-build-test COMMAND fable-defs-build-test)
endif()

install(TARGETS fable-defs-compiler
    EXPORT fable-defs-compiler-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS fable-defc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(EXPORT fable-defs-compiler-targets
    FILE fable-defs-compiler-targets.cmake
    NAMESPACE fable::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fable-defs-compiler)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fable-defs-compilerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defs-compilerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fable-defs-compiler)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defs-compilerConfigVersion.cmake
    VERSION 0.0.0
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defs-compilerConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defs-compilerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fable-defs-compiler)
