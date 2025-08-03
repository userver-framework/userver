# userver Framework Project Brief

## Project Overview

**userver** is an open source asynchronous framework with a rich set of abstractions for fast and comfortable creation of C++ microservices, services and utilities.

The framework solves the problem of efficient I/O interactions transparently for developers. Operations that would typically suspend the thread of execution do not suspend it. Instead, the thread processes other requests and tasks and returns to the handling of the operation only when it is guaranteed to execute immediately.

This approach results in straightforward source code, avoids CPU-consuming context switches from OS, and efficiently utilizes the CPU with a small amount of execution threads.

## Key Features

- Efficient asynchronous drivers for databases (MongoDB, PostgreSQL, Redis/Valkey, ClickHouse, MySQL/MariaDB, YDB, SQLite) and data transfer protocols (HTTP/{1.1, 2.0}, gRPC, AMQP 0-9-1, Kafka, TCP, TLS, WebSocket)
- Rich set of high-level components for caches, tasks, distributed locking, logging, tracing, statistics, metrics, JSON/YAML/BSON
- Functionality to change the service configuration on-the-fly
- On-the-fly configurable drivers, options of the deadline propagation, timeouts, congestion-control
- Comprehensive set of asynchronous low-level synchronization primitives and OS abstractions

## Purpose

The userver framework is designed to help developers create high-performance C++ microservices and applications with minimal effort. It abstracts away the complexity of asynchronous programming while providing excellent performance characteristics.

## Target Use Cases

- High-performance microservices
- Real-time data processing systems
- API gateways and service meshes
- Distributed systems requiring efficient I/O handling
- Applications requiring integration with multiple databases and messaging systems