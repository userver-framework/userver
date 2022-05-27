# userver <img src="./scripts/docs/logo.svg" align='right' width="30%">

**userver** is an open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.

The framework solves the problem of efficient I/O interactions transparently for
the developers. Operations that would typically suspend the thread of
execution do not suspend it. Instead of that, the thread processes other
requests and tasks and returns to the handling of the operation only when it is
guaranteed to execute immediately: 

```cpp
std::size_t InsertKey(storages::postgres::ClusterPtr pg, std::string_view key) {
    // Asynchronous execution of the SQL query. Current thread handles other
    // requests while the response from the DB is being received:
    auto res = pg->Execute(storages::postgres::ClusterHostType::kMaster,
                           "INSERT INTO key_table (key) VALUES ($1)", key);
    return res.RowsAffected();
}
```

As a result, with the framework you get straightforward source code,
avoid CPU-consuming context switches from OS, efficiently
utilize the CPU with a small amount of execution threads.


## Other Features

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


[See the docs for more info](https://userver-framework.github.io/).
