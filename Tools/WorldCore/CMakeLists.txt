cmake_minimum_required(VERSION 3.16)
project(TheThugWorldLogic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(../SaveCore ${CMAKE_CURRENT_BINARY_DIR}/SaveCore)
add_library(the_thug_world STATIC WorldCore.cpp)
target_link_libraries(the_thug_world PUBLIC the_thug_party_save)
add_executable(world_tests tests.cpp)
target_link_libraries(world_tests PRIVATE the_thug_world)
add_test(NAME world_core COMMAND world_tests)
