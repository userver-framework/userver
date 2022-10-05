# userver <img src="./scripts/docs/logo.svg" align='right' width="30%">

**userver** is an open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.

The framework solves the problem of efficient I/O interactions transparently for
the developers. Operations that would typically suspend the thread of 
execution do not suspend it. Instead of that, the thread processes other
requests and tasks and returns to the handling of the operation only when it is
guaranteed to execute immediately: 

```cpp
std::size_t Ins(storages::postgres::Transaction& tr, std::string_view key) {
  // Asynchronous execution of the SQL query in transaction. Current thread
  // handles other requests while the response from the DB is being received:
  auto res = tr.Execute("INSERT INTO keys VALUES ($1)", key);
  return res.RowsAffected();
}
```

As a result, with the framework you get straightforward source code,
avoid CPU-consuming context switches from OS, efficiently
utilize the CPU with a small amount of execution threads.


You can learn more about history and key features of userver from our articles 
on [Medium](https://medium.com/p/d5d9c4204dc2) (English) 
or [Habr](https://habr.com/post/674902) (Russian).

## Other Features

* Efficient asynchronous drivers for databases (MongoDB, PostgreSQL, Redis, ClickHouse, ...)
  and data transfer protocols (HTTP, GRPC, AMQP 0-9-1 (EXPERIMENTAL), TCP, ...), tasks construction and
  cancellation.
* Rich set of high-level components for caches, tasks, distributed locking,
  logging, tracing, statistics, metrics, JSON/YAML/BSON.
* Functionality to change the service configuration on-the-fly.
* On-the-fly configurable drivers, options of the deadline propagation,
  timeouts, congestion-control.
* Comprehensive set of asynchronous low-level synchronization primitives and
  OS abstractions. 


[See the docs for more info](https://userver.tech/d6/d2f/md_en_index.html).
