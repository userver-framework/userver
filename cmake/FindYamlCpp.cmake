# - Check for the presence of YAML
#
# The following variables are set when YAML is found:
#  YAML_FOUND      = Set to true, if all components of YAML have been found.
#  YAML_INCLUDES   = Include path for the header files of YAML
#  YAML_LIBRARIES  = Link these to use YAML
#  YAML_LFLAGS     = Linker flags (optional)

if (NOT YAML_FOUND)

  if (NOT YAML_ROOT_DIR)
    set (YAML_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
  endif (NOT YAML_ROOT_DIR)

  ##_____________________________________________________________________________
  ## Check for the header files

  find_path (YAML_INCLUDES yaml-cpp/yaml.h yaml-cpp/node.h
    HINTS ${YAML_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
    PATH_SUFFIXES include
    )

  ##_____________________________________________________________________________
  ## Check for the library

  find_library (YAML_LIBRARIES yaml-cpp
    HINTS ${YAML_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
    PATH_SUFFIXES lib
    )

  ##_____________________________________________________________________________
  ## Actions taken when all components have been found

  find_package_handle_standard_args (YAML DEFAULT_MSG YAML_LIBRARIES YAML_INCLUDES)

  if (YAML_INCLUDES AND YAML_LIBRARIES)
    set (YAML_FOUND TRUE)
  else (YAML_INCLUDES AND YAML_LIBRARIES)
    set (YAML_FOUND FALSE)
    if (NOT YAML_FIND_QUIETLY)
      if (NOT YAML_INCLUDES)
	message (STATUS "Unable to find YAML header files!")
      endif (NOT YAML_INCLUDES)
      if (NOT YAML_LIBRARIES)
	message (STATUS "Unable to find YAML library files!")
      endif (NOT YAML_LIBRARIES)
    endif (NOT YAML_FIND_QUIETLY)
  endif (YAML_INCLUDES AND YAML_LIBRARIES)

  if (YAML_FOUND)
    if (NOT YAML_FIND_QUIETLY)
      message (STATUS "Found components for YAML")
      message (STATUS "YAML_ROOT_DIR  = ${YAML_ROOT_DIR}")
      message (STATUS "YAML_INCLUDES  = ${YAML_INCLUDES}")
      message (STATUS "YAML_LIBRARIES = ${YAML_LIBRARIES}")
    endif (NOT YAML_FIND_QUIETLY)
  else (YAML_FOUND)
    if (YAML_FIND_REQUIRED)
        message (FATAL_ERROR "Could not find YAML! Please install libyaml-cpp-dev (>= 1:0.6)")
    endif (YAML_FIND_REQUIRED)
  endif (YAML_FOUND)

  ## Compatibility setting
  set (YAML_CPP_FOUND ${YAML_FOUND})

  ##_____________________________________________________________________________
  ## Mark advanced variables

  mark_as_advanced (
    YAML_ROOT_DIR
    YAML_INCLUDES
    YAML_LIBRARIES
    )

endif (NOT YAML_FOUND)
