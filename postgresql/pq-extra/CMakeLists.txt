project(userver-pq-extra)

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

find_package(PostgreSQL REQUIRED)
find_package(PostgreSQLInternal REQUIRED)
find_package_required(OpenSSL "libssl-dev")
find_package(GssApi REQUIRED)

set(SOURCES
  pq_portal_funcs.c
  pq_portal_funcs.h
  pq_workaround.c
  pq_workaround.h
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    PostgreSQL
    PostgreSQLInternal
  PRIVATE
    ${LDAP_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    GssApi
)

target_compile_options(
  ${PROJECT_NAME} PUBLIC -Wno-ignored-attributes -Wno-pedantic)


file(GLOB _PG_CONFIG_HEADERS "${PostgreSQL_INCLUDE_DIR}/pg_config*.h")
foreach(_PG_CONFIG_HEADER ${_PG_CONFIG_HEADERS})
  if(EXISTS "${_PG_CONFIG_HEADER}")
    file(STRINGS "${_PG_CONFIG_HEADER}" pgsql_version_str
         REGEX "^#define[\t ]+PG_VERSION_NUM[\t ]+.*")
    if(pgsql_version_str)
      string(REGEX REPLACE "^#define[\t ]+PG_VERSION_NUM[\t ]+([0-9]*).*"
             "\\1" LIBPQ_VERSION_NUM "${pgsql_version_str}")
      break()
    endif()
  endif()
endforeach()
message("Found PostgreSQL version (PG_VERSION_NUM): ${LIBPQ_VERSION_NUM}")
