if(TARGET SASL2::SASL2)
# Maybe already found by mongoc-1.0 config
  return()
endif()

_userver_module_begin(
    NAME SASL2
    DEBIAN_NAMES libsasl2-dev
    FORMULA_NAMES cyrus-sasl
    PKG_CONFIG_NAMES libsasl2
)

_userver_module_find_include(
    NAMES sasl/sasl.h
    PATHS
      /usr/lib/x86_64-linux-gnu
      /opt/homebrew/include
)

# sasl2 has to many dependencies to reliably be linked statically
_userver_module_find_library(
    NAMES libsasl2.dylib libsasl2.tbd libsasl2.so
    PATHS
      /usr/lib/x86_64-linux-gnu
      /opt/homebrew/lib
)

_userver_module_end()

add_library(SASL2::SASL2 ALIAS SASL2)
