# μserver <img src="https://github.yandex-team.ru/taxi/userver/blob/develop/scripts/docs/logo.svg" align='right' width="30%">
Userver is an open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.

The problem of efficient I/O interactions is solved transparently for the developers:

```cpp
std::size_t InsertKey(storages::postgres::ClusterPtr pg, std::string_view key) {
    // Asynchronous execution of the SQL query. Current thread handles other
    // requests while the response from the DB is being received:
    auto res = pg->Execute(storages::postgres::ClusterHostType::kMaster,
                           "INSERT INTO key_table (key) VALUES ($1)", key);
    return res.RowsAffected();
}
```

Features:
* Efficient asynchronous drivers for databases (MongoDB, PostgreSQL, Redis, ...)
  and data transfer protocols (HTTP, GRPC, TCP, ...), tasks construction and
  cancellation.
* Rich set of high-level components for caches, tasks, distributed locking,
  logging, tracing, statistics, metrics, JSON/YAML/BSON.
* Functionality to change the service configuration on-the-fly.
* On-the-fly configurable drivers, options of the deadline propagation,
  timeouts, congestion-control.
* Comprehensive set of asynchronous low-level synchronization primitives and
  OS abstractions. 


[See the docs for more info](https://pages.github.yandex-team.ru/taxi/userver/).
