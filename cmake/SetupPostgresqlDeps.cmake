include_guard(GLOBAL)

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
