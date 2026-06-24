find_package(Doxygen)

if (DOXYGEN_FOUND)
  set(doxygen_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxygen_out ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
  set(DOXYGEN_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/")
  set(DOXYGEN_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")

  configure_file(${doxygen_in} ${doxygen_out} @ONLY)

  add_custom_target(
    docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxygen_out}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "loc generating API documentation with Doxygen"
  )
else()
  message(FATAL_ERROR "Doxygen could not be found")
endif()
