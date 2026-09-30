cmake_minimum_required(VERSION 3.16)
project(TheThugPartySave LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(../IntegrationCore ${CMAKE_CURRENT_BINARY_DIR}/IntegrationCore)

add_library(the_thug_party_save STATIC SaveCore.cpp)
target_link_libraries(the_thug_party_save PUBLIC the_thug_integration)

add_executable(save_tests tests.cpp)
target_link_libraries(save_tests PRIVATE the_thug_party_save)

add_test(NAME party_save COMMAND save_tests)
