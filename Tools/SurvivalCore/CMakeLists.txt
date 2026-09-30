cmake_minimum_required(VERSION 3.16)
project(TheThugSurvival LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(the_thug_survival STATIC SurvivalCore.cpp Inventory.cpp)
target_include_directories(the_thug_survival PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(survival_tests tests.cpp)
target_link_libraries(survival_tests PRIVATE the_thug_survival)

enable_testing()
add_test(NAME survival_core COMMAND survival_tests)
