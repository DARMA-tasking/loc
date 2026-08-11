cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

project(loc_tpl_smoke LANGUAGES CXX)

# Exercise loc exactly as an embedding project such as VT does. Because loc is
# a subproject here, its standalone dependencies and auxiliary targets default
# to disabled.
add_subdirectory("../.." loc)

if(LOC_ENABLE_COMM OR LOC_ENABLE_MPI)
  message(FATAL_ERROR "loc TPL defaults unexpectedly enabled comm or MPI")
endif()

get_target_property(_loc_tpl_links loc INTERFACE_LINK_LIBRARIES)
if(_loc_tpl_links MATCHES "comm::|MPI::")
  message(
    FATAL_ERROR
    "loc TPL target leaked a standalone dependency: ${_loc_tpl_links}"
  )
endif()

add_executable(loc_tpl_smoke main.cc)
target_link_libraries(loc_tpl_smoke PRIVATE loc::loc)

enable_testing()
add_test(NAME loc_tpl_smoke COMMAND loc_tpl_smoke)
