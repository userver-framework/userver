include_guard()

if(NOT DEFINED CPM_USE_NAMED_CACHE_DIRECTORIES)
  set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
endif()

include(get_cpm)

if(NOT COMMAND CPMAddPackage)
  message(
      FATAL_ERROR
      "Failed to find CPM to download a package. You can turn off "
      "USERVER_DOWNLOAD_PACKAGES to avoid automatic downloads."
  )
endif()
