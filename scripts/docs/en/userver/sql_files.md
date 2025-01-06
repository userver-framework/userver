# External SQL/YQL files

You may generate SQL queries or YQL queries (for YDB) from external .sql/.yql files.
To do this, call the following cmake function in your CMakeLists.txt:

@snippet samples/postgres_service/CMakeLists.txt Postgres service sample - CMakeLists.txt

It will generate the .hpp file with following variable:

@code{.hpp}
extern const USERVER_NAMESPACE::storages::Query kCreateTable;
@endcode

And the definition in .cpp looks something like that:

@code{.cpp}
const USERVER_NAMESPACE::storages::Query kCreateTable = {
    R"-(
    CREATE TABLE IF NOT EXISTS key_value_table (
            key VARCHAR PRIMARY KEY,
            value VARCHAR
    )
    )-",
    USERVER_NAMESPACE::storages::Query::Name("create_table"),
    USERVER_NAMESPACE::storages::Query::LogMode::kFull,
};
@endcode

You may use it as usual by passing to `storages::postgres::Cluster::Execute()`
for SQL files or `ydb::TableClient::ExecuteDataQuery()` for YQL files.
