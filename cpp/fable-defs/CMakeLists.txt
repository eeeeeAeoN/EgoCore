cmake_minimum_required(VERSION 3.16)

project(fable-defs VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(fable-defs INTERFACE)
add_library(fable::defs ALIAS fable-defs)
set_target_properties(fable-defs PROPERTIES EXPORT_NAME defs)
target_compile_features(fable-defs INTERFACE cxx_std_11)
target_include_directories(fable-defs
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

include(CTest)
if(BUILD_TESTING)
    add_executable(fable-defs-test tests/defs_test.cpp)
    target_link_libraries(fable-defs-test PRIVATE fable-defs)
    if(MSVC)
        target_compile_options(fable-defs-test PRIVATE /W4 /WX)
    else()
        target_compile_options(fable-defs-test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME fable-defs-test COMMAND fable-defs-test)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS fable-defs EXPORT fable-defs-targets)
install(EXPORT fable-defs-targets
    FILE fable-defs-targets.cmake
    NAMESPACE fable::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fable-defs)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fable-defsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fable-defs)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fable-defsConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fable-defs)
