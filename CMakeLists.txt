cmake_minimum_required(VERSION 3.24)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use a separate build directory.")
endif()

project(KeysMultitude VERSION 0.1.0 LANGUAGES CXX)

option(KEYS_MULTITUDE_BUILD_TESTS "Build and register library tests" "${PROJECT_IS_TOP_LEVEL}")
option(KEYS_MULTITUDE_INSTALL "Generate library installation rules" "${PROJECT_IS_TOP_LEVEL}")

add_library(keys_multitude INTERFACE)
add_library(KeysMultitude::keys_multitude ALIAS keys_multitude)
target_compile_features(keys_multitude INTERFACE cxx_std_20)
target_sources(keys_multitude INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES keyMultitude.hpp backends/vectorBackend.hpp
)

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
