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
find_package(Threads REQUIRED)

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
)

if(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
  find_package(libintl REQUIRED)
  target_link_libraries(PostgreSQLInternal INTERFACE libintl)
endif()

set(PQ_EXTRA_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/pq_portal_funcs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/pq_workaround.c
)

add_library(${PROJECT_NAME} STATIC ${PQ_EXTRA_SOURCES})

target_include_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    PostgreSQL
  PRIVATE
    PostgreSQLInternal
)

target_compile_options(
  ${PROJECT_NAME} PUBLIC -Wno-ignored-attributes -Wno-pedantic)

# Make try_compile honor parent CMAKE_C_STANDARD
cmake_policy(SET CMP0067 NEW)

try_compile(
  COMPILE_RESULT
  ${CMAKE_CURRENT_BINARY_DIR}/require_pq_extra
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/check_main.c
    ${PQ_EXTRA_SOURCES}
  LINK_LIBRARIES
    PostgreSQLInternal
  OUTPUT_VARIABLE COMPILE_OUTPUT
)

if(NOT "${COMPILE_RESULT}")
  message(FATAL_ERROR "Error while compiling libpq patch:\n"
      "${COMPILE_OUTPUT}\n"
      "If there are errors up above then the versions of libpq, "
      "libpgport and libpgcommon diverged: ${PQ_EXTRA_INITIAL_LIBRARIES_LIST}\n"
      "Install the same versions of PostgreSQL client and server libraries, "
      "use USERVER_PG_SERVER_INCLUDE_DIR, USERVER_PG_SERVER_LIBRARY_DIR, "
      "USERVER_PG_INCLUDE_DIR and USERVER_PG_LIBRARY_DIR CMake options "
      "if needed; or disable libpq patching via "
      "CMake flag -DUSERVER_FEATURE_PATCH_LIBPQ=OFF."
  )
endif()
