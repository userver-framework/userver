# userver: ODBC Driver Wrapper

ODBC storage wrapper for `userver` (cluster + connection pool + query execution).

API overview is under active development.

## Quick start

Create a `storages::odbc::Cluster` with ODBC DSN and execute a query:

```cpp
#include <userver/storages/odbc.hpp>

using namespace std::chrono_literals;

storages::odbc::settings::PoolSettings pool_settings{
    /*min_size=*/1,
    /*max_size=*/5,
};

storages::odbc::settings::HostSettings host_settings{
    /*dsn=*/"DRIVER={PostgreSQL Unicode};SERVER=localhost;PORT=15433;DATABASE=postgres;UID=testsuite;PWD=password;",
    /*pool=*/pool_settings,
};

storages::odbc::settings::ODBCClusterSettings cluster_settings{
    /*pools=*/{host_settings},
};

storages::odbc::Cluster cluster{cluster_settings};

auto rs = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
auto row = rs[0];
auto field = row[0];
// field.GetInt32() / GetInt64() / GetString() / ...
```

### Reading results

`Execute(...)` returns `storages::odbc::ResultSet`. Each row is `storages::odbc::Row`, and each field is `storages::odbc::Field`:

```cpp
auto rs = cluster.Execute(storages::odbc::ClusterHostType::kMaster,
                           "SELECT 42, 'test', 1.0, false, null, true");

const auto row = rs[0];
const auto i32 = row[0].GetInt32();
const auto str = row[1].GetString();
if (row[4].IsNull()) {
    // ...
}
```

## Deadlines

ODBC operations can be aborted when a deadline is reached.

### Explicit deadline

Use the overloads that accept `engine::Deadline`:

```cpp
#include <userver/engine/deadline.hpp>
#include <userver/storages/odbc.hpp>

using namespace std::chrono_literals;

auto deadline = engine::Deadline::FromDuration(200ms);
auto rs = cluster.Execute(deadline, storages::odbc::ClusterHostType::kMaster, "SELECT 1");
```

Deadlines are also applied to transactions started with `Begin(deadline, ...)`:

```cpp
auto tx = cluster.Begin(deadline, storages::odbc::ClusterHostType::kMaster);
auto rs = tx.Execute("SELECT 1");
tx.Commit();  // deadline is honored internally
```

### Deadline resolution in ODBC

ODBC driver statement timeout (`SQL_ATTR_QUERY_TIMEOUT`) is configured in whole seconds.
As a result, when converting `engine::Deadline` to the ODBC timeout, sub-second deadlines are rounded up to the next full second (so the operation may run slightly longer than the exact deadline).

### Request deadline propagation

If you call ODBC from a request task, the task-inherited request deadline is automatically merged into ODBC deadlines.
If it expires, `storages::odbc::OperationInterrupted` is thrown.

## Transactions

Transactions are created via `Cluster::Begin(...)`.
They execute statements via `Transaction::Execute(...)`, then finish with `Commit()` or `Rollback()`.
If neither commit nor rollback was called, the transaction rolls back on destruction (RAII).

```cpp
auto tx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);

tx.Execute("INSERT INTO t(a) VALUES (1)");
tx.Execute("UPDATE t SET a = a + 1 WHERE a = 1");

tx.Commit();
```

## Exceptions

Common exceptions from `userver::storages::odbc`:

- `storages::odbc::OperationInterrupted` — deadline expired
- `storages::odbc::ConnectionError` — connection / driver failures
- `storages::odbc::StatementError` — statement-level execution errors

