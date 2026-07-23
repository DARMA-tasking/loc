find_package(GTest QUIET)

if (NOT GTest_FOUND)
  include(FetchContent)
  # Keep the fetched googletest out of loc's own install rules.
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
  FetchContent_MakeAvailable(googletest)
endif()

# Setup tests

set(LOC_TESTS loc_tests)

file(GLOB TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(${LOC_TESTS} ${TEST_SOURCE_FILES})

target_link_libraries(${LOC_TESTS}
  PRIVATE
    loc
    GTest::gtest
)

target_include_directories(
  ${LOC_TESTS} PRIVATE ${LOC_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
)

# Single-process run (exercises local/same-rank paths).
add_test(NAME ${LOC_TESTS} COMMAND ${LOC_TESTS})

# Multi-rank run (exercises the cross-rank resolver protocol). MPIEXEC_* are
# provided by find_package(MPI).
if(MPIEXEC_EXECUTABLE)
  add_test(
    NAME ${LOC_TESTS}_mpi
    COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
            ${MPIEXEC_PREFLAGS} $<TARGET_FILE:${LOC_TESTS}> ${MPIEXEC_POSTFLAGS}
  )
  set_tests_properties(${LOC_TESTS}_mpi PROPERTIES TIMEOUT 120)
endif()
