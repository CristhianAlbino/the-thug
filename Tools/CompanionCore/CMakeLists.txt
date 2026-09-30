cmake_minimum_required(VERSION 3.16)
project(TheThugCompanions LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(the_thug_companions STATIC CompanionCore.cpp)
target_include_directories(the_thug_companions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(companion_tests tests.cpp)
target_link_libraries(companion_tests PRIVATE the_thug_companions)

enable_testing()
add_test(NAME companion_core COMMAND companion_tests)
