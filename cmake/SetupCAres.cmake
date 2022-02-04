if (NOT OPEN_SOURCE_BUILD)
    find_package_required(yandex-c-ares "libyandex-taxi-c-ares-dev")
    add_library(c-ares ALIAS yandex-c-ares)  # Unify link names
    return()
endif()

option(USERVER_FEATURE_CARES_DOWNLOAD "Download and setup c-ares if no c-ares of matching version was found" ${USERVER_FEATURE_DOWNLOAD_PACKAGES})

if (USERVER_FEATURE_CARES_DOWNLOAD)
    find_package(c-ares 1.16)
    if (NOT c-ares_FOUND)
      find_package(Externalc-ares REQUIRED)
      set(c-ares_VERSION "1.18.1" CACHE STRING "Version of the c-ares")
      return()
    endif()
else()
    find_package(c-ares 1.16 REQUIRED)
endif()
