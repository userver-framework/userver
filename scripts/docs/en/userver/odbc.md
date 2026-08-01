## ODBC Driver

🐙 **userver** provides an asynchronous client for SQL databases that expose an
ODBC interface. The driver integrates connection pools, transactions,
deadlines, tracing, metrics, dynamic configuration, secdist and DNS resolution
with the userver component system.

### Executing queries safely

Use `?` placeholders and pass values separately. The driver prepares the SQL
statement and passes every value separately to the ODBC binding API with
`SQLBindParameter`. userver does not interpolate values into SQL: the selected
ODBC driver handles their escaping and typing, so an untrusted value cannot
alter the query structure. A driver may still serialize bound values as SQL
literals internally.

@snippet odbc/tests/odbc_postgresql_test.cpp ODBC parameter binding

The variadic API supports booleans, signed integers, unsigned integers up to
`INT64_MAX`, floating point values, strings and string views. Larger unsigned
values are rejected instead of relying on driver-specific conversion outside
the portable SQL `BIGINT` range. Use `std::optional<T>` for a typed nullable
value, or `nullptr` when the ODBC driver can infer the parameter type from the
statement. The number of C++ arguments must match the number of `?`
placeholders.

`storages::odbc::Cluster::Execute` returns a storages::odbc::ResultSet. Its rows
contain storages::odbc::Field values that provide typed getters such as
`GetInt32`, `GetInt64`, `GetDouble`, `GetBool`, and `GetString`.
The result is fully materialized as an in-memory snapshot before the connection
returns to the pool, so it remains readable after another query, transaction
completion, or topology reload. Consequently, large or unbounded `SELECT`s use
memory proportional to the complete result. `ResultSet::Size()` is the number
of materialized rows; use `ResultSet::RowsAffected()` for DML row counts.

### Transactions

Transactions are created with storages::odbc::Cluster::Begin. They commit or
roll back explicitly and automatically roll back on destruction if left open.
Parameters are bound in transaction queries in exactly the same way:

@snippet odbc/tests/odbc_transaction_test.cpp ODBC transaction parameter binding

### Command control and deadlines

storages::odbc::CommandControl configures the connection-acquisition/network
timeout and statement timeout for an operation. Pass an
storages::odbc::OptionalCommandControl to `Cluster::Execute`, `Cluster::Begin`,
or `Transaction::Execute` to override the defaults. `Begin`, every transaction
`Execute`, and `Commit` get a fresh operation deadline: the earliest of the
network budget, statement timeout, and task-inherited request deadline.
Explicit and automatic rollback use an independent cleanup budget so an
expired operation budget cannot return a dirty connection to the pool.

ODBC `SQL_ATTR_QUERY_TIMEOUT` has whole-second resolution. The driver rounds a
positive sub-second value up when passing it to ODBC while retaining the exact
userver deadline for pool waits and pre-operation checks. Cancellation of a
blocking ODBC call itself depends on timeout support in the selected driver.

Deadline expiry is reported as storages::odbc::OperationInterrupted.
Connection and driver failures use storages::odbc::ConnectionError, and
statement preparation, binding, and execution failures use
storages::odbc::StatementError.

### Component configuration

Add components::Odbc under `components_manager.components`. The following
tested configuration obtains its DSN from secdist:

@snippet odbc/functional_tests/basic_chaos/static_config.yaml ODBC component config

The complete generated static-config schema, including the mutually exclusive
`dsn`, `pools`, and `secdist_alias` connection sources, is available on
components::Odbc.

All ODBC driver-manager and driver calls are synchronous and run on the task
processor selected by `blocking_task_processor`; if omitted, the global
blocking task processor is used. Production services may dedicate and size a
task processor for ODBC so slow driver calls do not contend with unrelated
blocking work. Exact sub-second cancellation is observed after a synchronous
driver call returns unless that driver implements its own timeout sooner.
At most five connection attempts per pool run concurrently. A temporary
startup outage does not discard successfully initialized connections or abort
the component; a background monitor retries until `min_pool_size` is restored.

For secdist, `odbc_settings.databases.<alias>` accepts either a `dsn` string or
a `hosts` array. A host can be a DSN string or an object with a `dsn` member.
Using secdist keeps credentials out of the static configuration and supports
live credential/endpoint updates.

The `dns_resolver` static option selects `async` (the default, userver DNS
resolver) or `getaddrinfo` (resolution in the ODBC driver).

### Dynamic configuration and metrics

@ref USERVER_ODBC_DEFAULT_COMMAND_CONTROL controls default network and
statement timeouts. @ref USERVER_ODBC_CONNECTION_POOL_SETTINGS describes pool
settings by component name and the `__default__` fallback. Their schemas and
defaults are generated from the dynamic-config YAML sources and included in
the dynamic-config reference.

The component exports pool, query, error, timeout, and transaction statistics
under the `odbc` metric prefix, labelled with the component and pool.

@section odbc_info More information

- For component options and the generated schema, see components::Odbc.
- For query execution, see storages::odbc::Cluster.
- For result traversal, see storages::odbc::ResultSet.
- For transaction semantics, see storages::odbc::Transaction.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/sqlite/design_and_details.md | @ref scripts/docs/en/userver/mongodb.md ⇨
@htmlonly </div> @endhtmlonly
