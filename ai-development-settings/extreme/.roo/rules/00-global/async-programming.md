# Async Programming - Global Rules

**Rule ID**: `global.async.programming`  
**Priority**: 1000  
**Scope**: framework-wide  
**Override**: false  
**Extends**: [`global.framework.fundamentals`](framework-fundamentals.md)

## Coroutine Safety Fundamentals

### Non-Blocking Operations Only
All I/O operations must be non-blocking in coroutine context:

```cpp
// ✅ Correct: Async database operation
auto result = co_await pg_cluster->Execute(
    userver::storages::postgres::ClusterHostType::kMaster,
    "SELECT * FROM users WHERE id = $1", user_id
);

// ✅ Correct: Async HTTP request
auto response = co_await http_client.CreateRequest()
    .get(url)
    .timeout(std::chrono::seconds(5))
    .perform();

// ❌ NEVER: Blocking operations in coroutines
std::this_thread::sleep_for(std::chrono::seconds(1));  // FORBIDDEN
auto file = std::ifstream("data.txt");                 // FORBIDDEN
```

**Memory Bank Reference**: [`memory-bank://main/async-programming#coroutine-safety`](../memory-bank/main/async-programming.md#coroutine-safety)

### Deadline Propagation
Always propagate deadlines through the call chain:

```cpp
userver::engine::Task ProcessRequestAsync(
    const Request& request,
    userver::engine::Deadline deadline) {
    
    // Propagate deadline to database operations
    auto user_data = co_await pg_cluster->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        deadline,  // Propagate deadline
        "SELECT * FROM users WHERE id = $1", request.user_id
    );
    
    // Propagate deadline to HTTP requests
    auto external_data = co_await http_client.CreateRequest()
        .get(external_url)
        .timeout(deadline)  // Use deadline for timeout
        .perform();
    
    co_return ProcessData(user_data, external_data);
}
```

**Cross-Reference**: [`concept://concurrency/deadline_propagation`](../memory-bank/main/async-programming.md#deadline-propagation)

## Task Management Patterns

### Task Creation and Lifecycle
Use engine tasks for concurrent operations:

```cpp
// Create and manage concurrent tasks
std::vector<userver::engine::Task<Result>> tasks;

for (const auto& item : items) {
    tasks.emplace_back(userver::engine::AsyncNoSpan([item, this]() -> Result {
        return ProcessItemAsync(item);
    }));
}

// Wait for all tasks to complete
std::vector<Result> results;
for (auto& task : tasks) {
    results.push_back(co_await task);
}
```

### Task Cancellation Handling
Handle cancellation gracefully:

```cpp
userver::engine::Task<void> LongRunningOperation() {
    try {
        while (!userver::engine::current_task::ShouldCancel()) {
            // Perform work in chunks
            auto chunk_result = co_await ProcessChunk();
            
            // Check for cancellation between chunks
            if (userver::engine::current_task::ShouldCancel()) {
                LOG_INFO() << "Operation cancelled, cleaning up";
                break;
            }
        }
    } catch (const userver::engine::TaskCancelledException&) {
        LOG_INFO() << "Task was cancelled";
        // Perform cleanup
        throw;  // Re-throw to propagate cancellation
    }
}
```

**Memory Bank Reference**: [`memory-bank://main/async-programming#task-cancellation`](../memory-bank/main/async-programming.md#task-cancellation)

## Synchronization Primitives

### Mutex Usage in Coroutines
Use engine-aware synchronization primitives:

```cpp
class ThreadSafeCache {
private:
    mutable userver::engine::Mutex mutex_;
    std::unordered_map<std::string, Data> cache_;
    
public:
    userver::engine::Task<Data> GetAsync(const std::string& key) const {
        std::lock_guard lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            co_return it->second;
        }
        
        // Load data asynchronously while holding lock
        auto data = co_await LoadDataAsync(key);
        cache_[key] = data;
        co_return data;
    }
};
```

### Semaphore for Resource Limiting
Control concurrent access to limited resources:

```cpp
class ConnectionPool {
private:
    userver::engine::Semaphore semaphore_;
    std::vector<std::unique_ptr<Connection>> connections_;
    
public:
    ConnectionPool(size_t max_connections) 
        : semaphore_(max_connections) {
        // Initialize connection pool
    }
    
    userver::engine::Task<Result> ExecuteQuery(const std::string& query) {
        // Acquire semaphore (blocks if no connections available)
        auto lock = co_await semaphore_.lock();
        
        auto connection = GetAvailableConnection();
        co_return co_await connection->ExecuteAsync(query);
        
        // Semaphore automatically released when lock goes out of scope
    }
};
```

**Cross-Reference**: [`pattern://concurrency/synchronization_primitives`](../memory-bank/main/async-programming.md#synchronization)

## Error Handling in Async Context

### Exception Propagation
Exceptions propagate correctly through coroutines:

```cpp
userver::engine::Task<Result> AsyncOperation() {
    try {
        auto data = co_await FetchDataAsync();
        auto processed = co_await ProcessDataAsync(data);
        co_return processed;
    } catch (const NetworkError& e) {
        LOG_ERROR() << "Network error in async operation: " << e.what();
        throw;  // Exception propagates to caller
    } catch (const ProcessingError& e) {
        LOG_ERROR() << "Processing error: " << e.what();
        // Transform exception
        throw ServiceError("Failed to process data: " + std::string(e.what()));
    }
}
```

### Timeout Handling
Implement proper timeout handling:

```cpp
userver::engine::Task<Result> OperationWithTimeout(
    std::chrono::milliseconds timeout) {
    
    try {
        // Create task with timeout
        auto task = userver::engine::AsyncNoSpan([this]() {
            return LongRunningOperation();
        });
        
        // Wait with timeout
        co_return co_await userver::engine::WaitFor(task, timeout);
        
    } catch (const userver::engine::WaitInterruptedException&) {
        LOG_WARNING() << "Operation timed out after " << timeout.count() << "ms";
        throw TimeoutError("Operation exceeded timeout");
    }
}
```

**Memory Bank Reference**: [`memory-bank://main/async-programming#error-handling`](../memory-bank/main/async-programming.md#error-handling)

## Performance Optimization

### Task Processor Selection
Choose appropriate task processors for different workloads:

```yaml
# static_config.yaml
task_processors:
    main-task-processor:
        worker_threads: 4          # CPU-intensive tasks
        thread_name: main-worker
        
    fs-task-processor:
        worker_threads: 2          # File system operations
        thread_name: fs-worker
        
    monitor-task-processor:
        worker_threads: 1          # Monitoring and metrics
        thread_name: monitor
```

```cpp
// Use specific task processor for CPU-intensive work
auto cpu_task = userver::engine::AsyncNoSpan(
    userver::engine::TaskProcessor::Get("main-task-processor"),
    [data]() {
        return CpuIntensiveProcessing(data);
    }
);

// Use FS task processor for file operations
auto file_task = userver::engine::AsyncNoSpan(
    userver::engine::TaskProcessor::Get("fs-task-processor"),
    [filename]() {
        return ReadFileAsync(filename);
    }
);
```

### Batch Operations
Batch operations to reduce context switching:

```cpp
userver::engine::Task<std::vector<Result>> BatchProcess(
    const std::vector<Item>& items) {
    
    constexpr size_t kBatchSize = 100;
    std::vector<Result> results;
    results.reserve(items.size());
    
    // Process in batches
    for (size_t i = 0; i < items.size(); i += kBatchSize) {
        auto batch_end = std::min(i + kBatchSize, items.size());
        
        std::vector<userver::engine::Task<Result>> batch_tasks;
        for (size_t j = i; j < batch_end; ++j) {
            batch_tasks.emplace_back(ProcessItemAsync(items[j]));
        }
        
        // Wait for batch completion
        for (auto& task : batch_tasks) {
            results.push_back(co_await task);
        }
        
        // Yield between batches to allow other tasks to run
        co_await userver::engine::Yield();
    }
    
    co_return results;
}
```

## Memory Management in Async Context

### RAII with Coroutines
Ensure proper resource cleanup:

```cpp
class AsyncResourceManager {
private:
    std::unique_ptr<Resource> resource_;
    
public:
    AsyncResourceManager() = default;
    
    userver::engine::Task<void> InitializeAsync() {
        resource_ = co_await CreateResourceAsync();
        // Resource automatically cleaned up in destructor
    }
    
    ~AsyncResourceManager() {
        // Synchronous cleanup in destructor
        if (resource_) {
            resource_->Cleanup();
        }
    }
    
    userver::engine::Task<Result> UseResourceAsync() {
        if (!resource_) {
            throw std::runtime_error("Resource not initialized");
        }
        
        co_return co_await resource_->ProcessAsync();
    }
};
```

### Shared State Management
Manage shared state safely in async context:

```cpp
class AsyncCounter {
private:
    mutable userver::engine::Mutex mutex_;
    std::atomic<int> counter_{0};
    
public:
    userver::engine::Task<int> IncrementAsync() {
        // Atomic operations are safe without mutex
        return counter_.fetch_add(1) + 1;
    }
    
    userver::engine::Task<void> ComplexOperationAsync() {
        std::lock_guard lock(mutex_);
        
        // Complex operation requiring exclusive access
        auto current = counter_.load();
        co_await SomeAsyncOperation(current);
        counter_.store(current + 1);
    }
};
```

## Testing Async Code

### Unit Testing Coroutines
Test async functions properly:

```cpp
TEST(AsyncTest, BasicCoroutineTest) {
    userver::engine::RunStandalone([&] {
        auto result = AsyncFunction().Get();
        EXPECT_EQ(expected_value, result);
    });
}

TEST(AsyncTest, TimeoutTest) {
    userver::engine::RunStandalone([&] {
        EXPECT_THROW(
            AsyncFunctionWithTimeout(std::chrono::milliseconds{1}).Get(),
            TimeoutError
        );
    });
}
```

### Integration Testing with Tasks
Test concurrent operations:

```cpp
TEST(AsyncIntegrationTest, ConcurrentOperations) {
    userver::engine::RunStandalone([&] {
        std::vector<userver::engine::Task<int>> tasks;
        
        for (int i = 0; i < 10; ++i) {
            tasks.emplace_back(userver::engine::AsyncNoSpan([i] {
                return ProcessAsync(i).Get();
            }));
        }
        
        std::vector<int> results;
        for (auto& task : tasks) {
            results.push_back(task.Get());
        }
        
        EXPECT_EQ(10, results.size());
    });
}
```

**Memory Bank Reference**: [`memory-bank://main/service-patterns#testing-async`](../memory-bank/main/service-patterns.md#testing-async)

## Cross-References

### Related Memory Bank Entries
- [`memory-bank://main/async-programming`](../memory-bank/main/async-programming.md) - Comprehensive async patterns
- [`memory-bank://main/framework-core#concurrency`](../memory-bank/main/framework-core.md#concurrency) - Core concurrency concepts
- [`memory-bank://research/performance-research#async-optimization`](../memory-bank/research/performance-research.md#async-optimization) - Performance research

### Implementation Examples
- [`example://async/basic_coroutine`](../../samples/hello_service/) - Basic coroutine usage
- [`example://async/concurrent_processing`](../../samples/production_service/) - Concurrent task management
- [`example://async/database_operations`](../../samples/postgres_service/) - Async database patterns

### Alternative Approaches
- [`alternative://sync_vs_async`](../memory-bank/main/troubleshooting-guide.md#sync-async-migration) - Migration strategies
- [`alternative://task_vs_thread`](../memory-bank/research/new-patterns.md#concurrency-models) - Concurrency model choices

---

**Inheritance**: Extends framework fundamentals with async-specific rules.  
**Dependencies**: [`global.framework.fundamentals`](framework-fundamentals.md)  
**Validation**: All async code must follow these patterns.  
**Last Updated**: 2025-01-05  
**Next Review**: 2025-04-05