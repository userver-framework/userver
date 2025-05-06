# External SQL/YQL files

You may generate SQL queries or YQL queries (for YDB) from external .sql/.yql files.
To do this, call the following cmake function in your CMakeLists.txt:

@snippet samples/postgres_service/CMakeLists.txt Postgres service sample - CMakeLists.txt

It will generate the `samples_postgres_service/sql_queries.hpp` file with following variable:

@code{.hpp}
namespace samples_postgres_service::sql {

extern const USERVER_NAMESPACE::storages::Query kDeleteValue;

extern const USERVER_NAMESPACE::storages::Query kInsertValue;

extern const USERVER_NAMESPACE::storages::Query kSelectValue;

}
@endcode

And the definition for each statement in `samples_postgres_service/sql_queries.cpp` looks something like that:

@code{.cpp}
const USERVER_NAMESPACE::storages::Query kSelectValue = {
    R"-(
    SELECT value FROM key_value_table WHERE key=$1
    )-",
    USERVER_NAMESPACE::storages::Query::Name("select_value"),
    USERVER_NAMESPACE::storages::Query::LogMode::kFull,
};
@endcode

You may use it as usual by passing to `storages::postgres::Cluster::Execute()`
for SQL files or `ydb::TableClient::ExecuteDataQuery()` for YQL files:

@code{.cpp}
#include <samples_postgres_service/sql_queries.hpp>

namespace samples_postgres_service {
    ...
    auto result = trx->Execute(sql::kCreateTable);
    ...
}
@endcode

While writing tests, you can check the coverage of your SQL/YQL queries using the `sql_coverage` plugin.

To use it, you need to pass the target with generated queries to the `userver_testsuite_add_simple` (or `userver_testsuite_add`) function
in your CMakeLists.txt:

@snippet samples/postgres_service/CMakeLists.txt Postgres sql coverage - CMakeLists.txt

It will enable the `sql_coverage` plugin and add coverage test that will run with the other tests.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/chaotic.md | @ref scripts/docs/en/userver/testing.md ⇨
@htmlonly </div> @endhtmlonly
