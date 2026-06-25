set(LOC_EXAMPLES loc_examples)

file(GLOB EXAMPLE_SOURCES "${LOC_EXAMPLES_DIR}/*.cc")

foreach(EXAMPLE_FULL ${EXAMPLE_SOURCES})
  get_filename_component(EXAMPLE ${EXAMPLE_FULL} NAME_WE)

  add_executable(${EXAMPLE} ${LOC_EXAMPLES_DIR}/${EXAMPLE}.cc)

  target_link_libraries(${EXAMPLE} PRIVATE loc)
  target_link_libraries(${EXAMPLE} PRIVATE fmt)

  target_include_directories(${EXAMPLE} PRIVATE ${LOC_SOURCE_DIR})
endforeach()
