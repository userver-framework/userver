# - Check for the presence of YAML
#
# The following variables are set when YAML is found:
#  YAMLCPP_FOUND      = Set to true, if all components of YAML have been found.
#  YAMLCPP_INCLUDE_DIR   = Include path for the header files of YAML
#  YAMLCPP_LIBRARIES  = Link these to use YAML

if (NOT YAMLCPP_FOUND)

  ##_____________________________________________________________________________
  ## Check for the header files
  find_path (YAMLCPP_INCLUDE_DIR yaml-cpp/yaml.h yaml-cpp/node.h
    HINTS ${YAMLCPP_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
    PATH_SUFFIXES include
    )

  ##_____________________________________________________________________________
  ## Check for the library
  find_library (YAMLCPP_LIBRARIES yaml-cpp
    HINTS ${YAMLCPP_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
    PATH_SUFFIXES lib
    )

  ##_____________________________________________________________________________
  ## Actions taken when all components have been found
  find_package_handle_standard_args (YamlCpp
      REQUIRED_VARS YAMLCPP_LIBRARIES YAMLCPP_INCLUDE_DIR
      FAIL_MESSAGE "Could not find YAML! Please install libyaml-cpp-dev (>= 1:0.6)"
      )

  ##_____________________________________________________________________________
  ## Mark advanced variables
  mark_as_advanced (
    YAMLCPP_ROOT_DIR
    YAMLCPP_INCLUDE_DIR
    YAMLCPP_LIBRARIES
    )

endif (NOT YAMLCPP_FOUND)
