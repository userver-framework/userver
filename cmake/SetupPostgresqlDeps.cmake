include_guard(GLOBAL)

_userver_macos_set_default_dir(USERVER_PG_INCLUDE_DIR "pg_config;--includedir")
_userver_macos_set_default_dir(USERVER_PG_LIBRARY_DIR "pg_config;--libdir")
_userver_macos_set_default_dir(USERVER_PG_SERVER_INCLUDE_DIR "pg_config;--includedir-server")
_userver_macos_set_default_dir(USERVER_PG_SERVER_LIBRARY_DIR "pg_config;--pkglibdir")
_userver_macos_set_default_dir(OPENSSL_ROOT_DIR "brew;--prefix;openssl")

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
