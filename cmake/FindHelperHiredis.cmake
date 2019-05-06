# Hiredis' config doesn't set _FOUND :(
find_package(Hiredis 0.13.3)

if (HIREDIS_LIBRARIES AND NOT TARGET Hiredis)
  add_library(Hiredis INTERFACE)
  target_link_libraries(Hiredis INTERFACE ${HIREDIS_LIBRARIES})
  target_include_directories(Hiredis INTERFACE ${HIREDIS_INCLUDE_DIRS})
endif()
