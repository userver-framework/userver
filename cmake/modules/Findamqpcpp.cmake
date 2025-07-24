_userver_module_begin(
    NAME
    amqpcpp
    VERSION
    4.3.15
    FORMULA_NAMES
    amqp-cpp
    PACMAN_NAMES
    amqp-cpp
    PKG_CONFIG_NAMES
    amqpcpp
)

_userver_module_find_include(NAMES amqpcpp.h)

_userver_module_find_library(NAMES amqpcpp)

_userver_module_end()
