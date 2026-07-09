find_package(GTest QUIET)

if (NOT GTest_FOUND)
  include(FetchContent)
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
    GTest::gtest_main
)

target_include_directories(${LOC_TESTS} PRIVATE ${LOC_SOURCE_DIR})
