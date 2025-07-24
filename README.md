# userver [<img src="./scripts/docs/img/logo.svg" align='right' width="10%">](https://userver.tech/)

[![Ubuntu](https://github.com/userver-framework/userver/actions/workflows/ci.yml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/ci.yml)
[![MacOS](https://github.com/userver-framework/userver/actions/workflows/macos.yml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/macos.yml)
[![Alpine](https://github.com/userver-framework/userver/actions/workflows/alpine.yml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/alpine.yml)
[![Docker CI](https://github.com/userver-framework/userver/actions/workflows/docker.yaml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/docker.yaml)
[![Conan](https://github.com/userver-framework/userver/actions/workflows/ci-conan.yml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/ci-conan.yml)
[![CodeQL](https://github.com/userver-framework/userver/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/codeql-analysis.yml)

[![Build and publish ubuntu-24.04-userver images](https://github.com/userver-framework/userver/actions/workflows/publish-ubuntu-24.04-images.yaml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/publish-ubuntu-24.04-images.yaml)
[![Build and publish ubuntu-22.04-userver images](https://github.com/userver-framework/userver/actions/workflows/publish-ubuntu-22.04-images.yaml/badge.svg)](https://github.com/userver-framework/userver/actions/workflows/publish-ubuntu-22.04-images.yaml)

[![uservice-dynconf CI](https://github.com/userver-framework/uservice-dynconf/actions/workflows/ci.yml/badge.svg)](https://github.com/userver-framework/uservice-dynconf/actions/workflows/ci.yml)
[![uservice-dynconf Docker build](https://github.com/userver-framework/uservice-dynconf/actions/workflows/docker.yaml/badge.svg?branch=develop)](https://github.com/userver-framework/uservice-dynconf/actions/workflows/docker.yaml)
[![urealmedium CI](https://github.com/userver-framework/realmedium_sample/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/userver-framework/realmedium_sample/actions/workflows/ci.yml)
[![urealmedium Docker build](https://github.com/userver-framework/realmedium_sample/actions/workflows/docker.yaml/badge.svg?branch=develop)](https://github.com/userver-framework/realmedium_sample/actions/workflows/docker.yaml)


**userver** is an open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.

The framework solves the problem of efficient I/O interactions transparently for
the developers. Operations that would typically suspend the thread of 
execution do not suspend it. Instead of that, the thread processes other
requests and tasks and returns to the handling of the operation only when it is
guaranteed to execute immediately: 

```cpp
#include <userver/easy.hpp>
#include "schemas/key_value.hpp"

int main(int argc, char* argv[]) {
    using namespace userver;

    easy::HttpWith<easy::PgDep>(argc, argv)
        // Handles multiple HTTP requests to `/kv` URL concurrently
        .Get("/kv", [](formats::json::Value request_json, const easy::PgDep& dep) {
            // JSON parser and serializer are generated from JSON schema by userver
            auto key = request_json.As<schemas::KeyRequest>().key;

            // Asynchronous execution of the SQL query in transaction. Current thread
            // handles other requests while the response from the DB is being received:
            auto res = dep.pg().Execute(
                storages::postgres::ClusterHostType::kSlave,
                // Query is converted into a prepared statement. Subsequent requests
                // send only parameters in a binary form and meta information is
                // discarded on the DB side, significantly saving network bandwidth.
                "SELECT value FROM key_value_table WHERE key=$1", key
            );

            schemas::KeyValue response{key, res[0][0].As<std::string>()};
            return formats::json::ValueBuilder{response}.ExtractValue();
        });
}
```

As a result, with the framework you get straightforward source code,
avoid CPU-consuming context switches from OS, efficiently
utilize the CPU with a small amount of execution threads.


You can learn more about history and key features of userver from our 
[publications and videos](https://userver.tech/dc/d30/md_en_2userver_2publications.html).

## Other Features

* Efficient asynchronous drivers for databases (MongoDB, PostgreSQL, Valkey,
  Redis, ClickHouse, MySQL/MariaDB, YDB, SQLite ...) and data transfer protocols
  (HTTP/{1.1, 2.0}, gRPC, AMQP 0-9-1, Kafka, TCP, TLS,
  WebSocket ...), tasks construction and cancellation.
* Rich set of high-level components for caches, tasks, distributed locking,
  logging, tracing, statistics, metrics, JSON/YAML/BSON.
* Functionality to change the service configuration on-the-fly.
* On-the-fly configurable drivers, options of the deadline propagation,
  timeouts, congestion-control.
* Comprehensive set of asynchronous low-level synchronization primitives and
  OS abstractions. 


[See the docs for more info](https://userver.tech/de/d6a/md_en_2index.html).
