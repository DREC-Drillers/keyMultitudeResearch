cmake_minimum_required(VERSION 3.24)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use a separate build directory.")
endif()

project(KeysMultitude VERSION 0.1.0 LANGUAGES CXX)

find_package(GeneralFunctions CONFIG REQUIRED COMPONENTS tree)

option(KEYS_MULTITUDE_BUILD_TESTS "Build and register library tests" "${PROJECT_IS_TOP_LEVEL}")
option(KEYS_MULTITUDE_BUILD_EXECUTABLE "Build the keyMultitude executable" "${PROJECT_IS_TOP_LEVEL}")
option(KEYS_MULTITUDE_INSTALL "Generate library installation rules" "${PROJECT_IS_TOP_LEVEL}")
option(KEYS_MULTITUDE_BUILD_BENCHMARKS "Build Google Benchmark benchmarks" OFF)

add_library(keys_multitude INTERFACE)
add_library(KeysMultitude::keys_multitude ALIAS keys_multitude)
target_compile_features(keys_multitude INTERFACE cxx_std_20)
target_link_libraries(keys_multitude INTERFACE GeneralFunctions::tree)
# Use the adjacent working copy while developing the library and its consumers.
set(KEYS_MULTITUDE_GENERAL_FUNCTIONS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../generalFunctions"
    CACHE PATH "Optional GeneralFunctions source directory")
if(EXISTS "${KEYS_MULTITUDE_GENERAL_FUNCTIONS_SOURCE_DIR}/tree/avlTree.hpp")
    target_include_directories(keys_multitude BEFORE INTERFACE
        "$<BUILD_INTERFACE:${KEYS_MULTITUDE_GENERAL_FUNCTIONS_SOURCE_DIR}>")
endif()
target_sources(keys_multitude INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES keyMultitude.hpp backends/vectorBackend.hpp backends/setBackend.hpp backends/bTreeBackend.hpp backends/AVLTreeBackend.hpp
          backends/RBTreeBackend.hpp
)

if(KEYS_MULTITUDE_BUILD_EXECUTABLE)
    add_executable(keyMultitude main.cpp)
    target_link_libraries(keyMultitude PRIVATE KeysMultitude::keys_multitude)
    set_target_properties(keyMultitude PROPERTIES CXX_EXTENSIONS OFF)
endif()

if(KEYS_MULTITUDE_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
    set(benchmark_btree_suite "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/suites/btree-degree.toml")
    set(benchmark_degree_reader "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/btree_degrees.py")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${benchmark_btree_suite}" "${benchmark_degree_reader}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${benchmark_degree_reader}" "${benchmark_btree_suite}"
        OUTPUT_VARIABLE BENCHMARK_BTREE_DEGREES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    configure_file(benchmarks/btreeDegrees.hpp.in generated/btreeDegrees.hpp @ONLY)
    add_executable(keys_multitude_benchmarks
        benchmarks/main.cpp
        benchmarks/registerWorkload.cpp)
    target_include_directories(keys_multitude_benchmarks PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_link_libraries(keys_multitude_benchmarks PRIVATE
        KeysMultitude::keys_multitude benchmark::benchmark)
    set_target_properties(keys_multitude_benchmarks PROPERTIES CXX_EXTENSIONS OFF)
endif()

if(KEYS_MULTITUDE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(keys_multitude_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/KeysMultitude")
    install(TARGETS keys_multitude EXPORT KeysMultitudeTargets
        FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(EXPORT KeysMultitudeTargets
        NAMESPACE KeysMultitude::
        DESTINATION "${keys_multitude_package_dir}")
    configure_package_config_file(cmake/KeysMultitudeConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/KeysMultitudeConfig.cmake"
        INSTALL_DESTINATION "${keys_multitude_package_dir}")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/KeysMultitudeConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMinorVersion ARCH_INDEPENDENT)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/KeysMultitudeConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/KeysMultitudeConfigVersion.cmake"
        DESTINATION "${keys_multitude_package_dir}")
endif()

# Developer warnings stay private: consumers inherit only headers and C++20.
if(KEYS_MULTITUDE_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
