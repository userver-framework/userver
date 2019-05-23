find_path(FMT_INCLUDE_DIRECTORIES "fmt/format.h")
find_library(FMT_LIBRARIES fmt)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fmt
    "Can't find libfmt-dev, please install libfmt-dev or brew install fmt"
    FMT_INCLUDE_DIRECTORIES FMT_LIBRARIES)

if (Fmt_FOUND)
    if (NOT TARGET Fmt)
        add_library(Fmt INTERFACE IMPORTED)
    endif()
    set_property(TARGET Fmt PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIRECTORIES}")
    set_property(TARGET Fmt PROPERTY
        INTERFACE_LINK_LIBRARIES "${FMT_LIBRARIES}")
endif()

mark_as_advanced(FMT_INCLUDE_DIRECTORIES FMT_LIBRARIES)
