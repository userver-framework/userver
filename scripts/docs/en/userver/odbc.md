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
`INT64_MAX`, floating point values, strings and string views. It also provides
portable owning types for the corresponding standard ODBC values:

- storages::odbc::Bytes for `BINARY`, `VARBINARY`, and `LONGVARBINARY`;
- storages::odbc::Date and storages::odbc::Time for timezone-independent date
  and second-resolution time values;
- storages::odbc::Timestamp for a timezone-independent timestamp with a
  nanosecond fraction;
- `storages::odbc::Decimal<Precision, Scale>` for exact fixed-scale
  `DECIMAL`/`NUMERIC`, with portable precision from 1 to 38.

`Bytes`, date/time structures, timestamp structures, and numeric structures
are bound through their standard ODBC C and SQL types. `Decimal` requires exact
fixed-scale input (for example, `Decimal<9, 4>{"123.4500"}`), removes a leading
plus and redundant integer zeroes, preserves all Scale fractional digits, and
normalizes negative zero. Date, time, and timestamp have no timezone conversion
or implicit `system_clock` interpretation.

@snippet odbc/tests/odbc_postgresql_test.cpp ODBC portable standard types

Larger unsigned values are rejected instead of relying on driver-specific
conversion outside the portable SQL `BIGINT` range. Use `std::optional<T>` for
a typed nullable value, including every portable type above, or `nullptr` when
the ODBC driver can infer the parameter type from the statement. The number of
C++ arguments must match the number of `?` placeholders.

For queries assembled at runtime, storages::odbc::ParameterStore provides an
owning, ordered dynamic parameter list with the same supported value types and
the same safe ODBC binding path:

@snippet odbc/tests/odbc_postgresql_test.cpp ODBC dynamic parameter store

`PushBack` copies each value, so the store can outlive source objects and can be
reused by cluster and transaction executions without being consumed. Prefer an
empty `std::optional<T>` for NULL because `T` preserves the concrete binding
type. Raw `nullptr` and `std::nullopt` remain untyped and require driver type
inference; a null value whose static type is `const char*` is instead bound as
a typed string NULL.

For repeated DML with one SQL shape, storages::odbc::BulkParameterStore owns a
rectangular set of parameter rows. `ExecuteBulk` validates every row before
acquiring a connection, then executes bounded chunks using ODBC column-wise
parameter arrays where the driver supports the required statement attributes.
It safely falls back to scalar prepared executions when parameter arrays are
unsupported. Both paths use the same placeholders and portable value types:
`ExecuteBulk` accepts DML only and rejects statements that produce a result
set, including a result set revealed only after execution.

@snippet odbc/tests/odbc_bulk_test.cpp ODBC bulk DML

storages::odbc::BulkResult contains exactly one storages::odbc::BulkRowStatus
per requested row, an optional driver-reported processed count, the number of
rows whose status is trusted successful, and an optional aggregate affected-row
count. A failure after execution begins throws
storages::odbc::BulkExecutionError with this snapshot. Direct autocommit bulk
execution is intentionally not atomic: an earlier chunk or scalar fallback row
may already be committed. Use `Transaction::ExecuteBulk` and roll back the
transaction when all rows must succeed together. Each chunk shares one overall
operation deadline, trace, and metrics event; the default chunk size is
storages::odbc::kDefaultBulkRows. Drivers without trustworthy batch row-status
support may return kUnknown statuses even when the overall DML call succeeds;
the API never upgrades those rows into `Succeeded()` without reliable evidence.

`storages::odbc::Cluster::Execute` returns a storages::odbc::ResultSet. Its rows
contain storages::odbc::Field values. The compatibility getters `GetInt32`,
`GetInt64`, `GetDouble`, `GetBool`, and `GetString` remain available.
`Field::As<T>()` adds strict mapping for `bool`, every signed and unsigned
integer width, `float`, `double`, `std::string`, the portable ODBC types above,
and `std::optional<T>`. It rejects a mismatched SQL category, partial numeric
parse, target-width overflow, and SQL NULL for non-optional T. Decimal mapping
also requires the result column's reported precision and scale to exactly
match its `Decimal<Precision, Scale>` type.

Typed result helpers apply the same checks to complete rows:

@snippet odbc/tests/odbc_types_test.cpp ODBC typed result mapping

A scalar mapping requires exactly one column. A flat, public,
standard-layout aggregate is initialized in member declaration order and
requires exactly one column per member; nested aggregates and reference
members are rejected. `AsSingleRow` requires exactly one row, while
`AsOptionalSingleRow` accepts zero or one. If T itself is optional, the outer
optional represents row presence and the inner optional represents SQL NULL.

The result is fully materialized as an in-memory snapshot before the connection
returns to the pool, so it remains readable after another query, transaction
completion, or topology reload. Consequently, large or unbounded `SELECT`s use
memory proportional to the complete result. Binary values are materialized by
reported byte lengths rather than NUL terminators, so embedded zeroes and chunk
boundaries are preserved. `ResultSet::Size()` is the number of materialized
rows; use `ResultSet::RowsAffected()` for DML row counts.

### Transactions

Transactions are created with storages::odbc::Cluster::Begin. They commit or
roll back explicitly and automatically roll back on destruction if left open.
Parameters are bound in transaction queries in exactly the same way:

@snippet odbc/tests/odbc_transaction_test.cpp ODBC transaction parameter binding

storages::odbc::TransactionOptions can request any portable ODBC isolation
level and `SQL_ATTR_ACCESS_MODE` read-only/read-write hints:

@snippet odbc/tests/odbc_transaction_test.cpp ODBC transaction options

Default-constructed options do not override either connection attribute, so
the selected driver's current defaults are preserved. Explicit isolation is
accepted only when the physical connection reports the requested level and
then applies it exactly; the driver never silently substitutes a weaker level.
ODBC defines read-only access mode as an intent/optimization hint. Applications
must not rely on it as authorization or assume that write statements will be
rejected by every driver/database combination.

### Command control and deadlines

storages::odbc::CommandControl configures the connection-acquisition/network
timeout and statement timeout for an operation. Pass an
storages::odbc::OptionalCommandControl to `Cluster::Execute`, `Cluster::Begin`,
or `Transaction::Execute` to override configured values. Each field is resolved
independently in this order: built-in/default, task-inherited HTTP handler
path and method, named query, explicit per-call command control. A higher layer
only replaces fields that it specifies.

@snippet odbc/tests/odbc_deadline_test.cpp ODBC named query command control

The query layer is used only when storages::odbc::Query has a name; a plain SQL
string creates an unnamed query and skips `USERVER_ODBC_QUERIES_COMMAND_CONTROL`.
`Begin` resolves default, handler, and explicit layers and captures that result
as the transaction base. Every `Transaction::Execute` reads the current named
query config afresh, including for transactions opened before a config update,
then overlays an explicit statement command control. `Begin`, every transaction
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
statement timeouts. @ref USERVER_ODBC_HANDLERS_COMMAND_CONTROL maps handler
paths and HTTP methods to partial overrides. @ref
USERVER_ODBC_QUERIES_COMMAND_CONTROL maps `Query` names to partial overrides.
Publishing an empty map removes all overrides from that layer. @ref
USERVER_ODBC_CONNECTION_POOL_SETTINGS describes pool settings by component
name and the `__default__` fallback. Their schemas and defaults are generated
from the dynamic-config YAML sources and included in the dynamic-config
reference.

The component exports pool, query, error, timeout, and transaction statistics
under the `odbc` metric prefix, labelled with the component and pool. Named
query latency and error metrics are opt-in:

@snippet odbc/tests/odbc_metrics_test.cpp ODBC named query metrics

`max_statement_metrics` and @ref
USERVER_ODBC_STATEMENT_METRICS_SETTINGS bound the number of retained named-query
names independently in every `odbc_pool`. Each name exports three metric series,
so a pool can export up to `3 * max_statement_metrics` series. The dynamic
dictionary resolves an exact component name before `__default__`, then falls
back to the static value. Zero disables accounting and clears retained names.
Only storages::odbc::Query with an explicit name contributes; SQL text and
unnamed statements never become labels. Events are delivered asynchronously
through a bounded queue, so export is eventual and metrics events may be dropped
under telemetry overload. The exported siblings are `statement_timings`,
`statement_executed`, and `statement_errors`, each labelled by `odbc_query`.

Prepared statement caching is separately opt-in and bounded per physical ODBC
connection:

@snippet odbc/tests/odbc_metrics_test.cpp ODBC prepared statement cache

The static `max_prepared_cache_size` option and @ref
USERVER_ODBC_PREPARED_STATEMENT_CACHE_SETTINGS select the maximum number of
retained statements on each connection. Dynamic configuration resolves an
exact component name before `__default__`, then falls back to the static value.
Zero disables and clears the cache; growing preserves entries and shrinking
evicts least-recently-used entries before the connection's next operation.

Only parameterized queries use the cache. Parameterless SQL continues through
`SQLExecDirect`. Cache keys are the exact case-sensitive SQL bytes; Query names
and parameter values are not keys. Results remain fully materialized, so they do
not retain cached statement handles. Drivers may invalidate prepared statements
at commit or rollback; the cache follows the ODBC cursor-behavior capabilities
and conservatively clears when a capability is unknown.

The pool-level metrics `queries.prepared-cache-hits`,
`queries.prepared-cache-misses`, `queries.prepared-cache-evictions`, and
`connections.prepared-statements` inherit only `component` and `odbc_pool`
labels. Misses count enabled parameterized lookups, including prepare failures.
Evictions count capacity, resize, error, and transaction-boundary invalidation;
disable and connection destruction clears are excluded.

@section odbc_info More information

- For component options and the generated schema, see components::Odbc.
- For query execution, see storages::odbc::Cluster.
- For result traversal, see storages::odbc::ResultSet.
- For transaction semantics, see storages::odbc::Transaction.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/sqlite/design_and_details.md | @ref scripts/docs/en/userver/mongodb.md ⇨
@htmlonly </div> @endhtmlonly
