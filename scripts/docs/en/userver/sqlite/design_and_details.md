## SQLite driver design and implementation details 

This document offers a guided tour through **`userver-sqlite`**.  
It describes the main design decisions, shows how the pieces fit together, and highlights the trade-offs we made to keep SQLite usable inside a coroutine-based, non-blocking service.

### Motivation and scope

SQLite is a lightweight, embedded, relational database that supports ACID transactions and various journal modes, including WAL, Rollback, and Memory. However, it is inherently synchronous, posing challenges for asynchronous, high-performance applications. The main goal of the `userver-sqlite` driver is to integrate SQLite into userver's asynchronous environment seamlessly, ensuring efficient resource utilization and preventing the main thread from blocking.

#### Design Challenges and Objectives

- **Asynchronous API**. Never block CPU threads that execute coroutines; blocking I/O is outsourced to a dedicated task-processor.
- **High concurrency on WAL**. Exploit the “many readers + one writer” semantics of WAL without leaking *database is locked* errors.
- **Flexible and type-safe C++ interface**. Support Variadic templates, aggregate decomposition, compile-time checks where possible; runtime checks elsewhere. Avoid transient string conversions when binding parameters or extracting.
- **Safe RAII transactions**. Ensure resource safety and automatic rollback through RAII-based transactions.
- **Convenient configuration**. Easily configurable through YAML configurations.
- **Built-in metrics and logging**. SQLite driver metrics on connections (read/write), query stats, and transaction stats available out of the box.

### Architectural Overview

At the highest level, interaction with the database occurs through the Client class. This class abstracts the complexity of managing connections, preparing and executing statements, and handling transactions. The key components involved include:

- **Client**: High-level API for user interactions, supporting methods like Execute, Begin, and transaction management (Transaction, Savepoint).
- **Connection Pool**: Manages multiple SQLite connections, implementing different strategies based on configuration and journal modes.
- **Connection**: Central class that wraps the native sqlite3 handler, manages statement caching (LRU), and tracks per-connection statistics.
- **Prepared Statements and ResultSet**: Manage parameter binding and result retrieval.

### Connection pools & locking strategy

| Journal mode                              | Pools created                      | Parallelism          |
| ----------------------------------------- | ---------------------------------- | -------------------- |
| `wal` (default)                           | `RO × N` + `RW × 1`                | N readers + 1 writer |
| `delete`, `truncate`, `persist`, `memory` | `exclusive_rw` (single connection) | Fully serialized     |
| *Read-only service*                       | `RO × N` only                      | Unlimited readers    |

The pool strategy dynamically initializes based on configurations, providing flexibility and robustness against common SQLite locking errors.

Detailed Explanation

- `WAL (Write-Ahead Logging) Mode`: The default mode provides optimal concurrency by allowing multiple concurrent readers along with a single writer. The userver-sqlite driver leverages this by creating two separate connection pools: a read-only pool (RO) containing multiple connections and a read-write pool (RW) limited to a single connection. This prevents the common SQLite issue of "database is locked" errors by efficiently managing reader and writer contention.

- `Rollback, Memory, Delete, Truncate, Persist Modes`: These modes do not provide the concurrency benefits of WAL mode. They inherently serialize database access, meaning that when a write operation occurs, it exclusively locks the entire database. To manage this behavior safely, the driver implements an exclusive_rw strategy, utilizing a single read-write connection to serialize all database access fully. This approach simplifies management and guarantees database consistency but limits parallelism.

Read-Only Service: For scenarios requiring only read operations, the driver initializes a read-only connection pool with multiple connections (RO × N). This setup maximizes parallelism for read-intensive workloads, avoiding unnecessary overhead from managing write-enabled connections.

#### OperationType and Connection Pool Hints

Each query or transaction executed through the client must specify an `OperationType`. This acts as a hint to determine the appropriate connection pool to handle the request:

- `OperationType::kReadOnly` routes the query to the read-only connection pool, optimizing concurrency for read-heavy workloads.

- `OperationType::kReadWrite` directs the query to the read-write connection pool, reserved primarily for write operations.

### Implementation Details

#### Connection Management

The `Connection` class encapsulates the `sqlite3` native structure, which manages the database connection lifecycle. It includes an LRU cache for prepared statements, allowing efficient reuse by simply binding new parameters each time. Connections also maintain statistics on executed queries and transactions.

#### Statement Preparation and Parameter Binding

Queries are managed through variadic templates, enabling convenient parameter passing directly in method calls. For more structured data, the driver supports decomposing arguments from aggregates (tuples or structs) using   `Boost.Pfr` and `std::apply`, enhancing usability and type safety.

#### ResultSet Execution

`ResultSet` - proxy object that designed to encapsulate SQLite's incremental retrieval model. It manages executing queries in a step-by-step manner using `sqlite3_step` and provides convenient methods to retrieve results in various formats:

- `AsVector(RowTag)`
- `AsVector(FieldTag)`
- `AsSingleRow`
- `AsSingleField`
- `AsOptionalSingleRow`
- `AsOptionalSingleField`

All retrieval methods internally utilize a unified template-based extraction mechanism.

#### Async Model

Given SQLite's synchronous nature, blocking operations (`sqlite3_open`, `sqlite3_close`, `sqlite3_step`) are executed in a dedicated task processor (blocking_task_processor) using built-in `engine::AsyncNoSpan`. This approach isolates potentially blocking calls from userver's main task processor, ensuring application responsiveness and performance.

```cpp
struct sqlite3* NativeHandler::OpenDatabase(const settings::SQLiteSettings& settings) {
    return engine::AsyncNoSpan(blocking_task_processor_, [&settings] {
        sqlite3* handler = nullptr;
        int ret_code = sqlite3_open_v2(settings.db_path.c_str(), &handler, settings.flags, nullptr);
        if (ret_code != SQLITE_OK) {
            sqlite3_close(handler);
            throw SQLiteException(sqlite3_errstr(ret_code), ret_code);
        }
        return handler;
    }).Get();
}
```

It is important to note that this approach, while effective in isolating blocking operations, is not ideal. Small CPU-bound tasks may inadvertently be executed on the blocking processor, slightly impacting performance. However, this compromise was consciously chosen due to SQLite's inherent synchronous nature. SQLite officially recommends using the WAL journal mode for concurrency and asynchronous operations, as it significantly reduces blolocking and optimizes parallel read-write scenarios. Therefore, the driver strongly favors WAL mode for best performance and concurrency.

#### Transactions and Savepoints

Transactions follow the RAII pattern, automatically rolling back on scope exit if not explicitly committed. Both `Transaction` and `Savepoint` classes provide interfaces identical to the main `Client` class, simplifying transaction management:

```cpp
// Transactional Insert-Once Key-Value Logic
storages::sqlite::Transaction trx = sqlite_client_->Begin(
    storages::sqlite::OperationType::kReadWrite,
    {}
);

auto res = trx.Execute(
    "INSERT OR IGNORE INTO key_value_table (key, value) VALUES (?, ?)",
    key,
    value
).AsExecutionResult();
if (res.rows_affected) {  // If successfully insert a new pair
    trx.Commit();
    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    return std::string{value};
}

auto trx_res = trx.Execute(  // The key was already
    "SELECT value FROM key_value_table WHERE key = ?",
    key
).AsSingleField<std::string>();
transaction.Rollback();
```

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/sqlite/supported_types.md | @ref scripts/docs/en/userver/mongodb.md ⇨
@htmlonly </div> @endhtmlonly
