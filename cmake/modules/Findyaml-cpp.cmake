_userver_module_begin(
    NAME yaml-cpp
    DEBIAN_NAMES libyaml-cpp-dev
    FORMULA_NAMES yaml-cpp
    PACMAN_NAMES yaml-cpp
    CPM_NAME yaml-cpp
    CPM_GITHUB_REPOSITORY jbeder/yaml-cpp
    CPM_GIT_TAG yaml-cpp-0.7.0
)

_userver_module_find_include(NAMES yaml-cpp/yaml.h yaml-cpp/node.h PATH_SUFFIXES include)

_userver_module_find_library(NAMES yaml-cpp PATH_SUFFIXES lib)

_userver_module_end()
