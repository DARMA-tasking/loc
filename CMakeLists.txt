cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

file(READ "VERSION" _loc_version_str)
string(STRIP "${_loc_version_str}" _loc_version_str)
project(loc VERSION ${_loc_version_str})

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
endif()

set(LOC_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(LOC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LOC_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_subdirectory(examples)
add_subdirectory(src)
add_subdirectory(tests)
