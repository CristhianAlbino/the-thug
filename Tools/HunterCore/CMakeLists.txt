cmake_minimum_required(VERSION 3.16)
project(TheThugGameRules LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(the_thug_rules STATIC HunterCore.cpp RunRules.cpp)
target_include_directories(the_thug_rules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(hunter_core_tests tests.cpp)
target_link_libraries(hunter_core_tests PRIVATE the_thug_rules)

add_executable(run_rules_tests run_rules_tests.cpp)
target_link_libraries(run_rules_tests PRIVATE the_thug_rules)

enable_testing()
add_test(NAME hunter_core COMMAND hunter_core_tests)
add_test(NAME run_rules COMMAND run_rules_tests)
