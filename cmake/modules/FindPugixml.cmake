_userver_module_begin(
  NAME Pugixml
  DEBIAN_NAMES libpugixml-dev
  FORMULA_NAMES pugixml
  PACMAN_NAMES pugixml
  PKG_CONFIG_NAMES pugixml
)

_userver_module_find_include(
  NAMES pugixml.hpp
)

_userver_module_find_library(
  NAMES pugixml libpugixml
)

_userver_module_end()
