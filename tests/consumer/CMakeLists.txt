cmake_minimum_required(VERSION 3.24)
project(KeysMultitudeConsumer LANGUAGES CXX)

find_package(KeysMultitude 0.1 CONFIG REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE KeysMultitude::keys_multitude)
enable_testing()
add_test(NAME installed_consumer COMMAND consumer)
