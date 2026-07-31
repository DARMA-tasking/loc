set(LOC_LIBRARY loc)

# loc is a header-only, template library (the location coordinator and its
# internal data structures are all templates), so it is modeled as an INTERFACE
# target. Consumers get the <loc/...> include path and the comm dependency
# transitively.

add_library(${LOC_LIBRARY} INTERFACE)

target_include_directories(
  ${LOC_LIBRARY} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

if(LOC_ENABLE_COMM)
  target_link_libraries(${LOC_LIBRARY} INTERFACE comm::comm)
endif()
