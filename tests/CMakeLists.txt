if(NOT TARGET GTest::gtest_main)
    find_package(GTest 1.17 CONFIG REQUIRED)
endif()

add_executable(key_multitude_tests key_multitude_test.cpp btree_queries_test.cpp
    avl_backend_test.cpp range_workload_test.cpp)
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
