project(sqldto-integration-tests)

include("${USERVER_ROOT_DIR}/cmake/UserverSql.cmake")
include("${USERVER_ROOT_DIR}/cmake/UserverEmbedFile.cmake")

set(MIGRATIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/input/migrations")

userver_add_sql_library(
    sqldto-integration-input
    NAMESPACE input
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/input/queries"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/input_generated"
    SQL_FILES *.sql
    DTO_DIALECT postgresql
    MIGRATIONS_DIR "${MIGRATIONS_DIR}"
    DUMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/input"
)

userver_embed_file(
    sqldto-integration-embed-schema
    NAME input/V001__schema.sql
    FILEPATH "${MIGRATIONS_DIR}/V001__schema.sql"
    HPP_FILENAME input_schema.sql
)

add_executable(sqldto-integration client_pgtest.cpp embed.cpp)

target_link_libraries(
    sqldto-integration PRIVATE userver-utest userver-postgresql userver-postgresql-utest sqldto-integration-input
                               sqldto-integration-embed-schema
)

userver_add_utest(
    NAME sqldto-integration
    DATABASES postgresql
    TEST_ENV POSTGRES_TEST_DSN=postgresql://testsuite@localhost:15433/postgres
)
