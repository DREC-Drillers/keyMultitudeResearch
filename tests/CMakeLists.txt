if(NOT TARGET GTest::gtest_main)
    find_package(GTest 1.17 CONFIG QUIET)
endif()

if(NOT TARGET GTest::gtest_main)
    include(FetchContent)
    # Same pinned private dependency as cacheResearch; consumers do not need it.
    set(INSTALL_GTEST OFF)
    set(BUILD_GMOCK OFF)
    if(MSVC)
        set(gtest_force_shared_crt ON)
    endif()
    FetchContent_Declare(googletest
        URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.18.0
        URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        TIMEOUT 60
        INACTIVITY_TIMEOUT 30
    )
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(key_multitude_tests key_multitude_test.cpp)
target_link_libraries(key_multitude_tests PRIVATE
    KeysMultitude::keys_multitude GTest::gtest_main)
set_target_properties(key_multitude_tests PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(key_multitude_tests PRIVATE /W4 /permissive- /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    target_compile_options(key_multitude_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
include(GoogleTest)
gtest_discover_tests(key_multitude_tests DISCOVERY_MODE PRE_TEST)
