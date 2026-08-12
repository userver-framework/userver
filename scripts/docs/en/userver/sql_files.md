# Embedding SQL/YQL files via userver_add_sql_library

You may generate SQL queries or YQL queries (for YDB) from `.sql` / `.yql` files.
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
    USERVER_NAMESPACE::storages::Query::NameLiteral("select_value"),
    USERVER_NAMESPACE::storages::Query::LogMode::kFull,
};
@endcode

Each variable is statically initialized (has no dynamic|runtime initialization), giving a protection against static
initialization order fiasco when those variables are used.

You may use it as usual by passing to @ref storages::postgres::Cluster::Execute() or @ref storages::clickhouse::Cluster
for SQL files or @ref ydb::TableClient::ExecuteDataQuery() for YQL files:

@code{.cpp}
#include <samples_postgres_service/sql_queries.hpp>

namespace samples_postgres_service {
    ...
    auto result = trx->Execute(sql::kCreateTable);
    ...
}
@endcode

@anchor sql_coverage_test_info
## SQL coverage test

While writing tests, you can check the coverage of your SQL/YQL queries using the
@ref pytest_userver.plugins.sql_coverage "sql_coverage" plugin.

To use it, you need to pass the target with generated queries to the `userver_testsuite_add_simple` (or `userver_testsuite_add`) function
in your CMakeLists.txt as `SQL_LIBRARY` parameter:

@snippet samples/postgres_service/CMakeLists.txt Postgres sql coverage - CMakeLists.txt

It will enable the `sql_coverage` plugin and add coverage test that will run with the other tests.

@anchor sql_dto
## PostgreSQL DTO codegen

Besides the raw `storages::Query` constants shown above, `userver_add_sql_library` can also
generate a typed C++ client from your PostgreSQL migrations and `.sql` files. You get:

- `struct`s and `enum class`es for the types in your schema;
- a `PgClient` interface with one method per query, with two implementations:
    - a `ClusterPgClient` that runs the queries on a @ref storages::postgres::Cluster;
    - a `MockPgClient` (based on gmock) for unit tests.

The typed client is added on top of the usual `sql_queries.hpp` / `sql_queries.cpp` by the same
`userver_add_sql_library` call — just pass the `DTO_DIALECT` argument.

### Enabling DTO codegen in your CMakeLists.txt

Add `DTO_DIALECT`, `MIGRATIONS_DIR` and `DUMP_DIR` to your `userver_add_sql_library` call:

@snippet samples/postgres_dto_service/CMakeLists.txt Postgres DTO service sample - CMakeLists.txt

Relevant arguments:

| Argument         | Meaning                                                                                   |
|------------------|-------------------------------------------------------------------------------------------|
| `DTO_DIALECT`    | Enables DTO codegen for the given dialect. Currently only `postgresql` is supported.      |
| `MIGRATIONS_DIR` | Directory with `.sql` migration files that define the schema. Required when `DTO_DIALECT` is set. |
| `DUMP_DIR`       | Directory where the schema dump `schema.dto.json` is stored, relative to `${CMAKE_CURRENT_SOURCE_DIR}`(defaults to `.`) |

The library target always links against `userver::core`; setting `DTO_DIALECT` additionally links it
against `userver::${DTO_DIALECT}`.

Conventional input layout:

```
schemas/postgresql/<db>/migrations/V0X__*.sql
queries/<group>/<query_name>.sql
```

Each query file's stem becomes the method name in CamelCase: `select_value.sql` → `SelectValue`.
The `<group>` subdirectory is ignored.

#### Generated files

For `NAMESPACE = <ns>`:

- `include/<ns>/pg_models.hpp` — `struct`s and `enum class`es for user-defined PostgreSQL types.
- `include/<ns>/pg_client.hpp` — abstract `PgClient` interface; one `const` virtual per query.
- `include/<ns>/pg_cluster.hpp` + `src/<ns>/pg_cluster.cpp` — `ClusterPgClient` over @ref storages::postgres::ClusterPtr.
- `include/<ns>/pg_mock.hpp` — `MockPgClient` based on gmock.
- `include/<ns>/sql_queries.hpp` + `src/<ns>/sql_queries.cpp` — the raw `storages::Query` constants described above.

All of them are compiled into the single library target defined by `userver_add_sql_library`.

### Schema dump workflow

To work out the C++ types, the generator needs the real PostgreSQL schema. A normal build does not
start a database for this — it reads a committed dump (`schema.dto.json`) from `DUMP_DIR`. The dump
stores a hash of your migrations and queries, so the generator can tell when it is out of date.

Once you change a migration or a query, the old dump no longer matches and the build fails. The
error message prints the exact command to refresh the dump; running it starts a temporary
PostgreSQL, reads the schema, and writes a fresh `schema.dto.json` to `DUMP_DIR`. So the loop is:

1. Add or edit a migration or a `.sql` query.
2. Start the build. It fails and prints the command to refresh `schema.dto.json`.
3. Run that printed command — it writes a fresh `schema.dto.json` to `DUMP_DIR`.
4. Build again — this time it uses the dump and succeeds.

Commit `schema.dto.json` together with the change that caused it, so other developers and CI build
without starting a database.

### Worked examples

The examples below come from `scripts/sqldto/tests/golden_tests/`. Browse `input/pg_queries/`
and `output/pg_queries/` for the full input/output pair.

#### Example 1: a composite type and an enum

Input — `migrations/V001__queries_schema.sql` (excerpt):

@code{.sql}
CREATE TYPE queries.user_status AS ENUM (
    'active',
    'blocked',
    'deleted'
);

CREATE TYPE queries.user_profile AS (
    display_name TEXT,
    age INTEGER,
    country VARCHAR(100),
    is_verified BOOLEAN
);
@endcode

Generated `pg_models.hpp`:

@code{.cpp}
enum class QueriesUserStatus {
    kActive,   // active
    kBlocked,  // blocked
    kDeleted,  // deleted
};

struct QueriesUserProfile {
    std::optional<std::string> display_name;
    std::optional<std::int32_t> age;
    std::optional<std::string> country;
    std::optional<bool> is_verified;
};
@endcode

Enum variants get `kPascalCase` names. Composite struct fields are always wrapped in
`std::optional<>` because PostgreSQL composite-type fields are nullable.

#### Example 2: a simple SELECT query

Input — `queries/get_user_by_id.sql`:

@code{.sql}
SELECT id, username, email, status, created_at
FROM queries.users
WHERE id = $1;
@endcode

Generated result struct (in `pg_client.hpp`) and the method on `PgClient`:

@code{.cpp}
struct GetUserByIdRow {
    std::optional<std::int64_t> id;
    std::optional<std::string> username;
    std::optional<std::string> email;
    std::optional<QueriesUserStatus> status;
    std::optional<storages::postgres::TimePointTz> created_at;
};

virtual std::vector<GetUserByIdRow>
GetUserById(HostType host_type, const std::optional<std::int64_t>& arg1) const = 0;
@endcode

A query that returns more than one column gets a generated `<Query>Row` aggregate with one field
per result column, named after the column. A query that returns a single column returns that
column's type directly, with no wrapper struct.

`arg1` is `$1`. The default cardinality is `many`, so the method returns a `std::vector<...>`.
`HostType` is an alias for @ref storages::postgres::ClusterHostTypeFlags — pass
`storages::postgres::ClusterHostType::kMaster` or `kSlave`.

#### Example 3: a query with annotations

Input — `queries/create_user.sql`:

@code{.sql}
-- @arg1: TEXT
-- @arg2: TEXT
INSERT INTO queries.users (username, email, status)
VALUES ($1, $2, $3)
RETURNING id, username, email, status, created_at;
@endcode

Generated method, returning a `CreateUserRow` struct (same fields as `GetUserByIdRow` above, since
the `RETURNING` list is the same):

@code{.cpp}
virtual std::vector<CreateUserRow>
CreateUser(HostType host_type,
           const std::string& arg1,
           const std::string& arg2,
           const std::optional<QueriesUserStatus>& arg3) const = 0;
@endcode

`@arg1: TEXT` and `@arg2: TEXT` pin the PostgreSQL types of `$1`, `$2` to `TEXT`; `$3` has no
annotation and its type is inferred. Use `@arg<N>` when the generator cannot infer a parameter's
type on its own (it fails the build asking you to annotate).

The annotation does not decide whether an argument is `std::optional<>`. That is a separate check:
the generator tries passing `NULL` for each parameter and sees if PostgreSQL accepts it. Here `$1`
and `$2` are `INSERT` values for `NOT NULL` columns, so they come out non-optional. The same value
used only in a `WHERE id = $1` clause would come out as `std::optional<...>`, even with a pinned type.

### Using the generated client

Build a `ClusterPgClient` from the cluster of a @ref components::Postgres component and call its
methods directly. From `samples/postgres_dto_service`:

@snippet samples/postgres_dto_service/main.cpp Postgres service sample - component constructor

@snippet samples/postgres_dto_service/main.cpp Postgres service sample - GetValue

In unit tests, depend on the `PgClient` interface instead and pass `MockPgClient` from `pg_mock.hpp`.

### PostgreSQL → C++ type mapping

The generator emits the same C++ types used by the PostgreSQL driver. See
@ref scripts/docs/en/userver/pg/types.md for the full semantics, including
@ref pg_timestamp "timestamps" and @ref pg_arrays "arrays".

| PostgreSQL type                                              | C++ type                                       |
|--------------------------------------------------------------|------------------------------------------------|
| `boolean`                                                    | `bool`                                         |
| `smallint`, `smallserial`                                    | `std::int16_t`                                 |
| `integer`, `serial`                                          | `std::int32_t`                                 |
| `bigint`, `bigserial`                                        | `std::int64_t`                                 |
| `real`                                                       | `float`                                        |
| `double precision`                                           | `double`                                       |
| `numeric(p,s)` / `decimal(p,s)`                              | decimal64::Decimal                             |
| `text`, `varchar(n)`, `character varying(n)`, `character(n)` | `std::string`                                  |
| `char`                                                       | `char`                                         |
| `bytea`                                                      | `std::string`                                  |
| `date`                                                       | storages::postgres::Date                       |
| `time`, `time without time zone`                             | storages::postgres::TimeOfDay                  |
| `time with time zone`, `timetz`                              | storages::postgres::TimeOfDayTz                |
| `timestamp`, `timestamp without time zone`                   | storages::postgres::TimePointWithoutTz         |
| `timestamp with time zone`, `timestamptz`                    | storages::postgres::TimePointTz                |
| `interval`                                                   | `std::chrono::microseconds`                    |
| `json`, `jsonb`                                              | formats::json::Value                           |
| `uuid`                                                       | `boost::uuids::uuid`                           |
| `int4range`                                                  | storages::postgres::IntegerRange               |
| `int8range`                                                  | storages::postgres::BigintRange                |
| `T[]`                                                        | `std::vector<T>`                               |
| user `CREATE TYPE ... AS ENUM`                               | `enum class` (PascalCase; variants prefixed `k`) |
| user `CREATE TYPE ... AS (...)`                              | `struct` (PascalCase; all fields `std::optional<>`) |

Nullable result columns and parameters are wrapped in `std::optional<T>`.

@anchor sql_dto_annotations
### Per-query annotations

Annotations are single-line SQL comments `-- @<name>[: value]` inside a `.sql` query file. At most
one directive per comment line. Unknown directives cause codegen to fail.

| Annotation           | Syntax                                       | Default                                       | Effect                                                                                                          |
|----------------------|----------------------------------------------|-----------------------------------------------|-----------------------------------------------------------------------------------------------------------------|
| `@no-dto`            | `-- @no-dto`                          | DTO method is generated | Skip codegen for this query. The raw `storages::Query` constant is still produced; no `PgClient` method. Use it for queries you run only as a raw `storages::Query`, or that the analyzer cannot process (e.g. dynamic SQL, or statements it cannot prepare). |
| `@cardinality: V`    | `-- @cardinality: one\|optional\|many\|void` | `many`                  | Shape of the return type, where `RowType` is the `<Query>Row` struct (multi-column) or the single column's type: `one` → `RowType`; `optional` → `std::optional<RowType>`; `many` → `std::vector<RowType>`. A query with no result columns always returns `void`. `void` is accepted but has no distinct effect — a query that does return columns is treated as `many`. |
| `@arg<N>: <pg_type>` | `-- @arg1: TEXT`                      | inferred from query     | Pin the PostgreSQL type of `$<N>` (1-based). Use it when the type can't be inferred. It does not change whether the argument is `std::optional<>`. |

Examples:

@code{.sql}
-- @cardinality: one
SELECT count(*) FROM queries.users;
@endcode

@code{.sql}
-- @arg1: TEXT
SELECT id FROM queries.users WHERE username = $1;
@endcode

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/chaotic.md | @ref scripts/docs/en/userver/testing.md ⇨
@htmlonly </div> @endhtmlonly
