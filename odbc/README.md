# userver: ODBC Driver Wrapper [WIP]

ODBC storage wrapper for `userver` (cluster + connection pool + query execution).

Under active development!

## Quick start

Create a `storages::odbc::Cluster` with ODBC DSN and execute a query:

```cpp
#include <userver/storages/odbc.hpp>

using namespace std::chrono_literals;

storages::odbc::settings::PoolSettings pool_settings{
    .min_size=1,
    .max_size=5,
};

storages::odbc::settings::HostSettings host_settings{
    .dsn="DRIVER={PostgreSQL Unicode};SERVER=localhost;PORT=15433;DATABASE=postgres;UID=testsuite;PWD=password;",
    .pool=pool_settings,
};

storages::odbc::settings::ODBCClusterSettings cluster_settings{
    .pools={host_settings},
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

## Component Configuration

The ODBC component can be configured in the static config file. Below is the full schema:

### Single Pool Configuration

```yaml
components_manager:
    components:
        odbc:
            dsn: "DRIVER={PostgreSQL Unicode};SERVER=localhost;PORT=5432;DATABASE=mydb;UID=user;PWD=password"
            min_pool_size: 1      # optional, default: 1
            max_pool_size: 10     # optional, default: 10
            dns_resolver: async   # optional, default: async (options: async, getaddrinfo)
```

### Multi-Pool Configuration

For master-replica setups or multiple database hosts:

```yaml
components_manager:
    components:
        odbc:
            dns_resolver: async
            pools:
              - dsn: "DRIVER={PostgreSQL Unicode};SERVER=master.db.local;PORT=5432;DATABASE=mydb;UID=user;PWD=password"
                min_pool_size: 2
                max_pool_size: 15
              - dsn: "DRIVER={PostgreSQL Unicode};SERVER=replica.db.local;PORT=5432;DATABASE=mydb;UID=user;PWD=password"
                min_pool_size: 1
                max_pool_size: 10
```

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `secdist_alias` | string | — | Name of the database in secdist config (for secure credential storage) |
| `dsn` | string | — | ODBC connection string (for single-pool mode) |
| `min_pool_size` | integer | 1 | Minimum number of connections kept in the pool |
| `max_pool_size` | integer | 10 | Maximum number of connections in the pool |
| `dns_resolver` | string | `async` | DNS resolution mode: `async` (non-blocking) or `getaddrinfo` (blocking) |
| `pools` | array | — | List of pool configurations (for multi-pool mode) |

### Secdist Integration

For secure credential storage, you can use secdist instead of putting DSN strings directly in the static config:

```yaml
components_manager:
    components:
        odbc:
            secdist_alias: my_database
            min_pool_size: 1
            max_pool_size: 10
            dns_resolver: async
```

The secdist JSON file should contain:

```json
{
    "odbc_settings": {
        "databases": {
            "my_database": {
                "dsn": "DRIVER={PostgreSQL Unicode};SERVER=localhost;PORT=5432;DATABASE=mydb;UID=user;PWD=secret"
            }
        }
    }
}
```

For multiple hosts (master-replica setup):

```json
{
    "odbc_settings": {
        "databases": {
            "my_database": {
                "hosts": [
                    "DRIVER={PostgreSQL Unicode};SERVER=master.db.local;PORT=5432;DATABASE=mydb;UID=user;PWD=secret",
                    "DRIVER={PostgreSQL Unicode};SERVER=replica.db.local;PORT=5432;DATABASE=mydb;UID=user;PWD=secret"
                ]
            }
        }
    }
}
```

### DNS Resolution

The `dns_resolver` option controls how hostnames in DSN strings are resolved:

- **`async`** (default): Uses userver's asynchronous DNS resolver. Hostnames are resolved at component startup and replaced with IP addresses in the DSN. This is non-blocking and recommended for production.

- **`getaddrinfo`**: Uses the system's blocking `getaddrinfo()` call. The DSN is passed to the ODBC driver as-is, and hostname resolution happens during connection establishment.

When using `async` mode, the SERVER/HOST parameter in your DSN will be automatically resolved to an IP address before connecting. This allows for proper integration with service discovery and DNS-based load balancing.


### Programmatic Access

You can access the current dynamic config values programmatically:

```cpp
#include <userver/storages/odbc/cluster.hpp>

// Get current default timeouts from dynamic config
auto network_timeout = cluster->GetDefaultNetworkTimeout();
auto statement_timeout = cluster->GetDefaultStatementTimeout();

if (network_timeout.has_value()) {
    // Use the configured timeout
}
```

