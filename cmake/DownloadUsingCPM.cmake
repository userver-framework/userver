include_guard()

if(NOT DEFINED CPM_USE_NAMED_CACHE_DIRECTORIES)
  set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
endif()

# Workaround for https://github.com/cpm-cmake/CPM.cmake/issues/505
if(${CMAKE_VERSION} VERSION_LESS "3.17.0")
  include(FetchContent)
endif()

include(get_cpm)

if(NOT COMMAND CPMAddPackage)
  message(
      FATAL_ERROR
      "Failed to find CPM to download a package. You can turn off "
      "USERVER_DOWNLOAD_PACKAGES to avoid automatic downloads."
  )
endif()

if(CMAKE_VERSION VERSION_LESS "3.25.0")
  message(WARNING "Please update to cmake 3.25+ to remove warnings from third-party libs")
endif()

# If A uses find_package(B), and we install A and B using CPM, then:
# 1. make sure to call write_package_stub in SetupB
# 2. make sure to call SetupB at the beginning of SetupA
function(write_package_stub PACKAGE_NAME)
  file(WRITE "${CMAKE_BINARY_DIR}/package_stubs/${PACKAGE_NAME}Config.cmake")
endfunction()

function(_list_subdirectories directory result_list)
  set(result "${directory}")
  get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    _list_subdirectories("${subdirectory}" partial_result)
    list(APPEND result ${partial_result})
  endforeach()
  set("${result_list}" ${result} PARENT_SCOPE)
endfunction()

function(mark_targets_as_system directory)
  if(NOT directory OR NOT EXISTS "${directory}")
    message(FATAL_ERROR "Trying to mark a non-existent subdirectory '${directory}' as SYSTEM")
  endif()
  _list_subdirectories("${directory}" subdirectories)
  foreach(subdirectory IN LISTS subdirectories)
    get_property(targets DIRECTORY "${subdirectory}" PROPERTY BUILDSYSTEM_TARGETS)
    # Disable warnings in public headers
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25.0")
      set_target_properties(${targets} PROPERTIES SYSTEM TRUE)
    endif()
    # Disable warnings in sources
    foreach(target IN LISTS targets)
      get_target_property(target_sources "${target}" SOURCES)
      if(target_sources)
        target_compile_options("${target}" PRIVATE -w)
      endif()
    endforeach()
  endforeach()
endfunction()
