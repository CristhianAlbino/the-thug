cmake_minimum_required(VERSION 3.16)
project(TheThugPerception LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET the_thug_rules)
  add_subdirectory(../HunterCore ${CMAKE_CURRENT_BINARY_DIR}/HunterCore)
endif()

add_library(the_thug_perception STATIC PerceptionCore.cpp)
target_include_directories(the_thug_perception PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../HunterCore)
target_link_libraries(the_thug_perception PUBLIC the_thug_rules)

add_executable(perception_tests tests.cpp)
target_link_libraries(perception_tests PRIVATE the_thug_perception)

enable_testing()
add_test(NAME perception_core COMMAND perception_tests)
