_userver_module_begin(
    NAME
    Boost
    DEBIAN_NAMES
    libboost-dev  
    FORMULA_NAMES
    boost
    PACMAN_NAMES
    boost
    PKG_CONFIG_NAMES
    boost
)

_userver_module_find_include(NAMES boost/version.hpp)

_userver_module_end()

if(NOT TARGET Boost::boost AND Boost_FOUND)
    add_library(Boost::boost INTERFACE)
    target_include_directories(Boost::boost INTERFACE ${Boost_INCLUDE_DIRS})
endif()