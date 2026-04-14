## ScyllaDB

The scylla asynchronous driver provides an interface to work with ScyllaDB
and Apache Cassandra databases using CQL

## Main features

* Type-safe operations for INSERT, SELECT, UPDATE, DELETE, COUNT via storages::scylla::operations;
* Raw CQL execution for hand-written queries, DDL, aggregations, system tables;
* Full CQL type coverage: text, int, bigint, boolean, float, double, uuid, timeuuid, timestamp, date, time, blob, inet, list\<T\>, set\<T\>, map\<K,V\>;
* Typed row deserialization via `Row::As<T>()` with user-defined `DecodeRow()`;
* Cursor-based and token-based paging for large result sets;
* Lightweight transactions (LWT): INSERT IF NOT EXISTS, UPDATE IF, DELETE IF EXISTS;
* USING TTL and USING TIMESTAMP on writes;
* Logged batch inserts via storages::scylla::operations::InsertMany;
* Configurable consistency levels, load balancing, speculative execution, retry policies;
* Connection pooling with shard awareness;
* TLS/SSL support;
* Congestion control;
* @ref scripts/docs/en/userver/deadline_propagation.md .


## Metrics

Most important ones:

| Metric name                     | Description                                          |
|---------------------------------|------------------------------------------------------|
| scylla.pool.current-size        | current number of open connections                   |
| scylla.pool.max-size            | limit on the number of open connections              |
| scylla.pool.overloads           | counter of requests that could not get a connection  |
| scylla.success                  | counter of successfully executed requests            |
| scylla.errors                   | counter of failed requests                           |
| scylla.timings                  | query timings                                        |

See @ref scripts/docs/en/userver/service_monitor.md for info on how to get the metrics.


## Usage

To use ScyllaDB you have to add the component components::Scylla and configure it
according to the documentation. After that you can get a session and work with
tables:

```cpp
#include <userver/storages/scylla/component.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/scylla/operations.hpp>

// Get session from the component
auto session = context.FindComponent<components::Scylla>("scylla-db").GetSession();

// Get a table handle
auto table = session->GetTable("users");
```


### Insert

```cpp
storages::scylla::operations::InsertOne op;
op.BindString("name", "Alice");
op.BindInt32("age", 30);
op.BindUuid("id", storages::scylla::Uuid::Random());
op.BindTimestamp("created_at", std::chrono::system_clock::now());
table.Execute(op);
```


### Insert with TTL

Rows inserted with TTL are automatically deleted after the specified duration:

```cpp
storages::scylla::operations::InsertOne op;
op.BindString("key", "session-token");
op.BindString("value", "abc123");
op.UsingTtl(3600);  // expires after 1 hour
table.Execute(op);
```

You can also set an explicit write timestamp (microseconds) for conflict
resolution:

```cpp
op.UsingTimestamp(1700000000000000);  // microseconds since epoch
```


### Select one row

```cpp
storages::scylla::operations::SelectOne op;
op.AddAllColumns();
op.WhereString("name", "Alice");

auto row = table.Execute(op);
if (!row.Empty()) {
    auto name = row.Get<std::string>("name");
    auto age = row.Get<std::int32_t>("age");
    auto id = row.Get<storages::scylla::Uuid>("id");
}
```


### Select many rows

```cpp
storages::scylla::operations::SelectMany op;
op.AddColumn("name");
op.AddColumn("age");
op.SetLimit(100);

auto rows = table.Execute(op);
for (const auto& row : rows) {
    // row is storages::scylla::Row
}
```


### Typed row deserialization

Define a struct and a `DecodeRow` function (found via ADL):

```cpp
struct User {
    storages::scylla::Uuid id;
    std::string name;
    std::int32_t age{0};
};

void DecodeRow(const storages::scylla::Row& row, User& out) {
    out.id = row.Get<storages::scylla::Uuid>("id");
    out.name = row.Get<std::string>("name");
    if (auto a = row.TryGet<std::int32_t>("age")) out.age = *a;
}

// Then use it:
auto user = row.As<User>();
```


### Paging

For large result sets, use `ExecutePaged` with an opaque paging state token:

```cpp
storages::scylla::operations::SelectMany op;
op.AddAllColumns();
op.SetPageSize(100);

std::string cursor;  // empty for first page
auto result = table.ExecutePaged(op, cursor);

// result.rows        — current page
// result.has_more_pages — true if more pages exist
// result.paging_state — pass back for next page
```


### Paging 

For server-side iteration (batch processing, exports), use `Session::NewCursor`:

```cpp
auto cursor = session->NewCursor("SELECT * FROM users", {}, /*page_size=*/1000);

while (!cursor.Done()) {
    auto page = cursor.NextPage();
    if (page) {
        for (const auto& row : *page) {
            // process row
        }
    }
}
```


### Update

```cpp
storages::scylla::operations::UpdateOne op;
op.SetInt32("age", 31);
op.WhereString("name", "Alice");
op.UsingTtl(86400);  // optional: updated columns expire after 24h
table.Execute(op);
```


### Delete

```cpp
storages::scylla::operations::DeleteOne op;
op.WhereString("name", "Alice");
table.Execute(op);
```


### Count

```cpp
storages::scylla::operations::Count op;
op.WhereString("status", "active");
auto count = table.Execute(op);  // returns std::int64_t
```


### Bulk insert (logged batch)

```cpp
storages::scylla::operations::InsertMany op;
op.BindString("name", "Alice");
op.BindInt32("age", 30);
op.NextRow();
op.BindString("name", "Bob");
op.BindInt32("age", 25);
table.Execute(op);
```


### Truncate

```cpp
storages::scylla::operations::Truncate op;
table.Execute(op);
```


### Lightweight transactions (LWT)

Conditional insert. succeeds only if the row does not exist:

```cpp
storages::scylla::operations::InsertOne op;
op.BindString("name", "Alice");
op.BindInt32("age", 30);
op.IfNotExists();

auto result = table.ExecuteLwt(op);
if (result.applied) {
    // row was inserted
} else {
    // row already existed; result.previous contains the existing row
}
```

Compare-and-set. update only if a column matches the expected value:

```cpp
storages::scylla::operations::UpdateOne op;
op.SetInt32("age", 31);
op.WhereString("name", "Alice");
op.IfInt32("age", 30);  // only apply if age is currently 30

auto result = table.ExecuteLwt(op);
// result.applied, result.previous
```

Conditional delete:

```cpp
storages::scylla::operations::DeleteOne op;
op.WhereString("name", "Alice");
op.IfExists();

auto result = table.ExecuteLwt(op);
```


### Raw CQL execution

For queries that don't fit the operation builders. DDL, aggregations,
system tables, complex WHERE clauses, ALLOW FILTERING:

```cpp
// SELECT with parameters
auto rows = session->Execute(
    "SELECT * FROM users WHERE name = ?",
    std::string{"Alice"});

// Variadic convenience
auto rows2 = session->Execute(
    "SELECT * FROM users WHERE age > ? AND age < ?",
    std::int64_t{20}, std::int64_t{40});

// DDL / DML (no result expected)
session->ExecuteVoid(
    "CREATE TABLE IF NOT EXISTS ks.cache ("
    "key text PRIMARY KEY, value text)");

// Paged raw query
auto paged = session->ExecutePaged(
    "SELECT * FROM users", {}, /*page_size=*/100);
```


### Rich CQL types

The driver supports the full CQL type system via storages::scylla::Value:

| CQL type         | C++ type                                   |
|------------------|--------------------------------------------|
| text / varchar   | `std::string`                              |
| int              | `std::int32_t`                             |
| bigint           | `std::int64_t`                             |
| smallint         | `std::int16_t`                             |
| tinyint          | `std::int8_t`                              |
| boolean          | `bool`                                     |
| float            | `float`                                    |
| double           | `double`                                   |
| uuid / timeuuid  | `storages::scylla::Uuid`                   |
| timestamp        | `storages::scylla::Timestamp` (= `std::chrono::system_clock::time_point`) |
| date             | `storages::scylla::Date`                   |
| time             | `storages::scylla::Time`                   |
| blob             | `storages::scylla::Blob` (= `std::vector<std::byte>`) |
| inet             | `storages::scylla::Inet`                   |
| list\<T\>        | `storages::scylla::List`                   |
| set\<T\>         | `storages::scylla::Set`                    |
| map\<K,V\>       | `storages::scylla::Map`                    |

Working with collection types:

```cpp
// Bind a set<text>
storages::scylla::Set tags;
tags.items.emplace_back(std::string{"prod"});
tags.items.emplace_back(std::string{"critical"});
op.BindSet("tags", std::move(tags));

// Bind a map<text, text>
storages::scylla::Map metadata;
metadata.entries.emplace_back(
    storages::scylla::Value{std::string{"env"}},
    storages::scylla::Value{std::string{"production"}});
op.BindMap("metadata", std::move(metadata));

// Bind a list<int>
storages::scylla::List scores;
scores.items.emplace_back(std::int32_t{95});
scores.items.emplace_back(std::int32_t{88});
op.BindList("scores", std::move(scores));
```

Reading collection types from a row:

```cpp
if (const auto* v = row.Find("tags"); v && v->Is<storages::scylla::Set>()) {
    for (const auto& item : v->Get<storages::scylla::Set>().items) {
        auto tag = item.Get<std::string>();
    }
}
```


### UUID generation

```cpp
auto random_id = storages::scylla::Uuid::Random();      // v4 random UUID
auto time_id   = storages::scylla::Uuid::TimeBased();    // v1 time-based UUID
auto parsed    = storages::scylla::Uuid::FromString("550e8400-e29b-41d4-a716-446655440000");
auto text      = random_id.ToString();
```


## Static configuration

```yaml
scylla-db:
    dbconnection: scylla               # secdist key
    consistency: local_quorum          # default consistency level
    serial_consistency: local_serial   # for LWT operations
    request_timeout: 10s
    pool_size: 16
    app_name: my_service
    shard_awareness: true              # token-aware routing
    retry_policy: default              # default / fallthrough
    load_balancing_policy: round_robin # round_robin / dc_aware
    speculative_execution:
        enabled: false
        max_attempts: 2
        delay: 100ms
    default_keyspace: my_keyspace
```


## Secdist configuration

```json
{
    "scylla_settings": {
        "scylla_example": {
            "hosts": "scylla-node-1,scylla-node-2,scylla-node-3"
        }
    }
}
```


## Error handling

All driver exceptions derive from `storages::scylla::ScyllaException`:

| Exception                    | When                                      |
|------------------------------|-------------------------------------------|
| `QueryException`             | CQL query error                           |
| `InvalidQueryArgumentException` | wrong bind type / missing parameter    |
| `NetworkException`           | connection lost                            |
| `ServerException`            | server-side error (code available)        |
| `TimeoutException`           | query timed out                           |
| `AuthenticationException`    | auth failure                              |
| `ClusterUnavailableException`| no nodes reachable                        |
| `PoolOverloadException`      | connection pool exhausted                 |
| `CancelledException`         | request cancelled / deadline propagation  |


## Demo

See @ref samples/scylla_service/README.md for a working key-value service that
demonstrates InsertOne, SelectOne, UpdateOne, DeleteOne, InsertMany, SelectMany,
Count, Truncate, and LWT operations (IF NOT EXISTS, Compare-And-Set, DELETE IF
EXISTS) over a `examples.basic` table.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/mongodb.md |
@htmlonly </div> @endhtmlonly
