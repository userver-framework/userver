_userver_module_begin(
    NAME yaml-cpp
    DEBIAN_NAMES libyaml-cpp-dev
    FORMULA_NAMES yaml-cpp
    PACMAN_NAMES yaml-cpp
    CPM_NAME yaml-cpp
    CPM_URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.7.0.tar.gz
    CPM_URL_HASH SHA256=43e6a9fcb146ad871515f0d0873947e5d497a1c9c60c58cb102a97b47208b7c3
)

_userver_module_find_include(NAMES yaml-cpp/yaml.h yaml-cpp/node.h PATH_SUFFIXES include)

_userver_module_find_library(NAMES yaml-cpp PATH_SUFFIXES lib)

_userver_module_end()

if(NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()
