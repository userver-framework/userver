include_guard(GLOBAL)

function(_set_default_pg_dir variable pg_config_args)
    if(NOT ${variable})
        execute_process(
            COMMAND pg_config ${pg_config_args}
            OUTPUT_VARIABLE output
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(${variable} ${output} PARENT_SCOPE)
    endif()
endfunction()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    _set_default_pg_dir(USERVER_PG_INCLUDE_DIR "--includedir")
    _set_default_pg_dir(USERVER_PG_LIBRARY_DIR "--libdir")
    _set_default_pg_dir(USERVER_PG_SERVER_INCLUDE_DIR "--includedir-server")
    _set_default_pg_dir(USERVER_PG_SERVER_LIBRARY_DIR "--pkglibdir")
endif()

# We need libldap to statically link with libpq
# There is no FindLdap.cmake and no package config files
# for ldap library, so need to search for it by hand.
find_library(LDAP_LIBRARY NAMES ldap)
if(NOT LDAP_LIBRARY)
  message(FATAL_ERROR "Failed to find libldap.so.\n"
          "The linux system ldap installs shared objects with very ugly names, "
          "so please install `libldap2-dev` package. "
          "For Mac OS X please install `openldap`.")
endif()

find_package(PostgreSQLInternal REQUIRED)
find_package(GssApi REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m sin "" USERVER_HAS_LIB_MATH)
set(USERVER_LIB_MATH)
if(USERVER_HAS_LIB_MATH)
  set(USERVER_LIB_MATH m)
endif()

get_target_property(PQ_EXTRA_INITIAL_LIBRARIES_LIST
    PostgreSQLInternal INTERFACE_LINK_LIBRARIES
)
target_link_libraries(PostgreSQLInternal
  INTERFACE
    ${LDAP_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    GssApi
    Threads::Threads
    ${USERVER_LIB_MATH}
)

if(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
  find_package(libintl REQUIRED)
  target_link_libraries(PostgreSQLInternal INTERFACE libintl)
endif()
