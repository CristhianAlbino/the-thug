cmake_minimum_required(VERSION 3.16)
project(TheThugIntegration LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(../CompanionCore ${CMAKE_CURRENT_BINARY_DIR}/CompanionCore)
add_subdirectory(../SurvivalCore ${CMAKE_CURRENT_BINARY_DIR}/SurvivalCore)

add_library(the_thug_integration STATIC IntegrationCore.cpp)
target_link_libraries(the_thug_integration PUBLIC
  the_thug_companions the_thug_survival)

add_executable(integration_tests tests.cpp)
target_link_libraries(integration_tests PRIVATE the_thug_integration)

add_test(NAME integration_core COMMAND integration_tests)
