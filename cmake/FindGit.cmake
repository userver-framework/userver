# FindGit
# -------
#
# The module defines the following variables:
#
# ::
#
#    GIT_EXECUTABLE - path to git command line client
#    GIT_FOUND - true if the command line client was found
#    GIT_VERSION_STRING - the version of git found (since CMake 2.8.8)
#
# Example usage:
#
# ::
#
#    find_package(Git)
#    if(GIT_FOUND)
#      message("git found: ${GIT_EXECUTABLE}")
#    endif()
#
# Look for 'git' or 'eg' (easy git)
#
set(git_names git eg)

find_program(GIT_EXECUTABLE
  NAMES ${git_names}
  PATHS ${github_path}
  PATH_SUFFIXES Git/cmd Git/bin
  DOC "git command line client"
  )
mark_as_advanced(GIT_EXECUTABLE)

if(GIT_EXECUTABLE)
  execute_process(COMMAND ${GIT_EXECUTABLE} --version
                  OUTPUT_VARIABLE git_version
                  ERROR_QUIET
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (git_version MATCHES "^git version [0-9]")
    string(REPLACE "git version " "" GIT_VERSION_STRING "${git_version}")
  endif()
  unset(git_version)
endif()

# Handle the QUIETLY and REQUIRED arguments and set GIT_FOUND to TRUE if
# all listed variables are TRUE

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Git
                                  REQUIRED_VARS GIT_EXECUTABLE
                                  VERSION_VAR GIT_VERSION_STRING)
