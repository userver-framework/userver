if(USERVER_SANITIZE)
  message(FATAL_ERROR
      "jemalloc is not compatible with sanitizers, "
      "please skip it for USERVER_SANITIZE-enabled builds"
  )
endif()

_userver_module_begin(
    NAME Jemalloc
    DEBIAN_NAMES libjemalloc-dev
    FORMULA_NAMES jemalloc
    RPM_NAMES jemalloc-devel
    PACMAN_NAMES jemalloc
    PKG_CONFIG_NAMES jemalloc
)

_userver_module_find_include(
    NAMES jemalloc/jemalloc.h
)

_userver_module_find_library(
    NAMES jemalloc
)

_userver_module_end()
