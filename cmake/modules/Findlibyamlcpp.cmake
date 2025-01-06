_userver_module_begin(
    NAME libyamlcpp
    DEBIAN_NAMES libyaml-cpp-dev
    FORMULA_NAMES yaml-cpp
    PACMAN_NAMES yaml-cpp
)

_userver_module_find_include(
    NAMES
    yaml-cpp/yaml.h
    yaml-cpp/node.h
    PATH_SUFFIXES include
)

_userver_module_find_library(
    NAMES yaml-cpp
    PATH_SUFFIXES lib
)

_userver_module_end()
