project(userver-pq-extra)

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
  PRIVATE
    PostgreSQLInternal
    ${LDAP_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    GssApi
)

target_compile_options(
  ${PROJECT_NAME} PUBLIC -Wno-ignored-attributes -Wno-pedantic)
