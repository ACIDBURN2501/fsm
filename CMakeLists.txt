# CMake minimum version
cmake_minimum_required(VERSION 3.14)

# Project definition
project(fsm
    VERSION 0.1.0
    DESCRIPTION "A modern C++ finite state machine library"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
include(CTest)
enable_testing()

# Export compile commands for clang-tidy / clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Library target (header‑only, but we provide an interface target)
add_library(fsm INTERFACE)
add_library(fsm::fsm ALIAS fsm)

# Header files for installation
set(FSM_HEADERS
    include/fsm/runtime.hpp
    include/fsm/version.hpp
)

# Include directories
target_include_directories(fsm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Doxygen documentation generation (optional)
find_package(Doxygen QUIET)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()

# ------------------------------------------------------------
# Testing using Catch2 (FetchContent)
# ------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0 # latest stable tag at time of writing
)
FetchContent_MakeAvailable(catch2)

add_executable(fsm_tests
    test/fsm_runtime_tests.cpp
    test/void_context_test.cpp
    test/int_type_test.cpp
    test/edge_cases_test.cpp
    test/dot_graph_test.cpp)
target_link_libraries(fsm_tests PRIVATE fsm Catch2::Catch2WithMain)
add_test(NAME fsm_tests COMMAND fsm_tests)

# Installation rules (header‑only)
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS fsm EXPORT fsmTargets)
install(EXPORT fsmTargets
    FILE fsmConfig.cmake
    NAMESPACE fsm::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fsm)

# Packaging (CPack) – optional, but set basic info
include(CPack)
set(CPACK_PACKAGE_NAME "fsm")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "TGZ")
