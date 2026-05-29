## ScyllaDB

The scylla asynchronous driver provides an interface to work with ScyllaDB
and Apache Cassandra databases using CQL

Under the hood it links the ScyllaDB [cpp-rs-driver](https://github.com/scylladb/cpp-rs-driver),
a Rust reimplementation of the DataStax C/C++ driver.

The framework wraps that C API in a thin
coroutine-friendly layer. Every `CassFuture` is bridged into an awaitable task,
so CQL calls suspend the current coroutine instead of blocking a task
processor thread, while the driver's own I/O threads keep talking to the cluster. 

## Main features

* Type-safe operations for INSERT, SELECT, UPDATE, DELETE, COUNT;
* Raw CQL execution for hand-written queries, DDL and aggregations
* Full CQL type coverage;
* Lightweight transactions (LWT)
* USING TTL and USING TIMESTAMP on writes;
* Logged batch inserts;
* @ref scripts/docs/en/userver/deadline_propagation.md .
* Configurable consistency levels, load balancing, speculative execution;


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
according to the documentation.

```cpp
#include <userver/storages/scylla/component.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/scylla/operations.hpp>

auto session = context.FindComponent<components::Scylla>("scylla-db").GetSession();
auto table = session->GetTable("users");
```


### Typed operations

Each CQL verb has a builder. Bind parameters with Bind, pin rows with
Where, stage updates with Set, then hand the builder to
Table::Execute

```cpp
storages::scylla::operations::InsertOne insert;
insert.BindString("name", "Alice");
insert.BindInt32("age", 30);
insert.UsingTtl(3600);
table.Execute(insert);

storages::scylla::operations::SelectOne select;
select.AddAllColumns();
select.WhereString("name", "Alice");
auto row = table.Execute(select);
```

Rows bound with UsingTtl expire after the given seconds;
UsingTimestamp sets an explicit write timestamp for conflict resolution.


### Typed row deserialization

```cpp
struct User {
    storages::scylla::Uuid id;
    std::string name;
};

void DecodeRow(const storages::scylla::Row& row, User& out) {
    out.id = row.Get<storages::scylla::Uuid>("id");
    out.name = row.Get<std::string>("name");
}

const auto user = row.As<User>();
```


### UUID's

```cpp
auto random_id = storages::scylla::Uuid::Random();
auto time_id   = storages::scylla::Uuid::TimeBased();
auto parsed    = storages::scylla::Uuid::FromString("550e8400-e29b-41d4-a716-446655440000");
auto text      = random_id.ToString();
```


### Paging

For large result sets, use ExecutePaged with an opaque paging token, or
Session::NewCursor for server-side iteration.

```cpp
storages::scylla::operations::SelectMany op;
op.AddAllColumns();
op.SetPageSize(100);

std::string cursor;
auto result = table.ExecutePaged(op, cursor);

auto server_cursor = session->NewCursor("SELECT * FROM users", {}, 1000);
while (!server_cursor.Done()) {
    auto page = server_cursor.NextPage();
}
```


### Lightweight transactions (LWT)

Conditional insert. Succeeds only if the row does not exist

```cpp
storages::scylla::operations::InsertOne op;

op.BindString("name", "Alice");
op.BindInt32("age", 30);
op.IfNotExists();

auto result = table.ExecuteLwt(op);
```

Compare-and-set. update only if a column matches the expected value

```cpp
storages::scylla::operations::UpdateOne op;

op.SetInt32("age", 31);
op.WhereString("name", "Alice");
op.IfInt32("age", 30);

auto result = table.ExecuteLwt(op);
```

Conditional delete:

```cpp
storages::scylla::operations::DeleteOne op;

op.WhereString("name", "Alice");
op.IfExists();

auto result = table.ExecuteLwt(op);
```


### Raw CQL execution

For queries that don't fit the operation builders.

```cpp
auto rows = session->Execute(
    "SELECT * FROM users WHERE name = ?",
    std::string{"Alice"});

auto rows2 = session->Execute(
    "SELECT * FROM users WHERE age > ? AND age < ?",
    std::int64_t{20}, std::int64_t{40});

session->ExecuteVoid(
    "CREATE TABLE IF NOT EXISTS ks.cache ("
    "key text PRIMARY KEY, value text)");

auto paged = session->ExecutePaged(
    "SELECT * FROM users", {}, 100);
```


### Rich CQL types

The driver supports the full CQL type system via storages::scylla::Value:

| CQL type         | C++ type                      |
|------------------|-------------------------------|
| text / varchar   | `std::string`                 |
| int              | `std::int32_t`                |
| bigint           | `std::int64_t`                |
| smallint         | `std::int16_t`                |
| tinyint          | `std::int8_t`                 |
| boolean          | `bool`                        |
| float            | `float`                       |
| double           | `double`                      |
| uuid / timeuuid  | `storages::scylla::Uuid`      |
| timestamp        | `storages::scylla::Timestamp` |
| date             | `storages::scylla::Date`      |
| time             | `storages::scylla::Time`      |
| blob             | `storages::scylla::Blob`      |
| inet             | `storages::scylla::Inet`      |
| list\<T\>        | `storages::scylla::List`      |
| set\<T\>         | `storages::scylla::Set`       |
| map\<K,V\>       | `storages::scylla::Map`       |


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/mongodb.md |
@htmlonly </div> @endhtmlonly
