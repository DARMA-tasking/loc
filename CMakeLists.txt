cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

file(READ "VERSION" _loc_version_str)
string(STRIP "${_loc_version_str}" _loc_version_str)
project(loc VERSION ${_loc_version_str})

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
endif()

set(_loc_standalone_default ${PROJECT_IS_TOP_LEVEL})
# include(cmake/load_doxygen.cmake)

option(
  LOC_ENABLE_COMM
  "Use the standalone comm communication backend"
  ${_loc_standalone_default}
)
option(
  LOC_ENABLE_MPI
  "Enable direct MPI support for examples and tests"
  ${_loc_standalone_default}
)
option(
  LOC_BUILD_EXAMPLES
  "Build loc examples"
  ${_loc_standalone_default}
)
option(
  LOC_BUILD_TESTS
  "Build loc tests"
  ${_loc_standalone_default}
)
option(
  LOC_BUILD_DOCS
  "Build loc API documentation"
  ${_loc_standalone_default}
)

unset(_loc_standalone_default)

if(LOC_BUILD_EXAMPLES AND (NOT LOC_ENABLE_COMM OR NOT LOC_ENABLE_MPI))
  message(
    FATAL_ERROR
    "LOC_BUILD_EXAMPLES requires LOC_ENABLE_COMM=ON and LOC_ENABLE_MPI=ON"
  )
endif()

# Optional standalone dependencies. When loc is added as a subdirectory (for
# example, as a TPL in VT), these are disabled by default and the embedding
# project provides the communication backend.
if(LOC_ENABLE_MPI)
  find_package(MPI REQUIRED COMPONENTS CXX)
endif()

if(LOC_ENABLE_COMM)
  find_package(comm REQUIRED CONFIG)
endif()

if(LOC_BUILD_DOCS)
  include(cmake/load_doxygen.cmake)
endif()

# Project components

set(LOC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(src)

if(LOC_BUILD_EXAMPLES)
  set(LOC_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
  add_subdirectory(examples)
endif()

if(LOC_BUILD_TESTS)
  set(LOC_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif()
