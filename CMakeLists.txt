cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

file(READ "VERSION" _loc_version_str)
string(STRIP "${_loc_version_str}" _loc_version_str)
project(loc VERSION ${_loc_version_str})

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
endif()

# include(cmake/load_doxygen.cmake)

# External libraries
#
# loc builds on the standalone `comm` communication library, which transitively
# provides MPI, magistrate/checkpoint, and fmt. The bundled lib/fmt is therefore
# not built here; fmt is consumed via comm.

find_package(MPI REQUIRED)
find_package(comm REQUIRED CONFIG)

# Project components

set(LOC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(src)

set(LOC_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
add_subdirectory(examples)

set(LOC_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
enable_testing()
add_subdirectory(tests)
