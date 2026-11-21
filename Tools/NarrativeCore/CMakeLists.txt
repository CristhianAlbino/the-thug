cmake_minimum_required(VERSION 3.16)
project(TheThugNarrativeCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(the_thug_narrative STATIC NarrativeCore.cpp)
target_include_directories(the_thug_narrative PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(narrative_tests narrative_tests.cpp)
target_link_libraries(narrative_tests PRIVATE the_thug_narrative)

enable_testing()
add_test(NAME narrative COMMAND narrative_tests)
