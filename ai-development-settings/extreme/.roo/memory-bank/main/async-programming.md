# Asynchronous Programming in Userver

## Overview

Userver leverages a coroutine-based asynchronous programming model to achieve high performance and scalability. This approach allows handling thousands of concurrent operations without the overhead of traditional threading models.

## Core Concepts

### Coroutines

Userver uses C++20 coroutines as the foundation for asynchronous operations. Coroutines allow writing asynchronous code that looks synchronous while maintaining non-blocking behavior.

#### Basic Coroutine Structure

```cpp
#include <userver/engine/task/task.hpp>

engine::Task MyAsyncFunction() {
    // Asynchronous operations
    co_await SomeAsyncOperation();
    
    // More operations
    co_return;
}
```

#### Coroutine Handles

Coroutines return `engine::Task` or `engine::TaskWithResult<T>` handles:

```cpp
// Fire-and-forget task
engine::Task task = MyAsyncFunction();

// Task with result
engine::TaskWithResult<int> result_task = AsyncFunctionWithResult();

// Wait for result
int result = result_task.Get();
```

### Engine System

The engine system manages the execution of coroutines and provides the underlying infrastructure for asynchronous operations.

#### Task Processor

The task processor manages coroutine execution on a thread pool:

```cpp
// Get current task processor
auto& task_processor = engine::current_task::GetTaskProcessor();

// Submit task to processor
task_processor.Spawn(MyAsyncFunction());
```

#### Scheduling

Coroutines are scheduled cooperatively, meaning they must yield control explicitly:

```cpp
// Yield control to scheduler
engine::Yield();

// Sleep for a duration
engine::SleepFor(std::chrono::milliseconds(100));

// Wait for deadline
engine::WaitForDeadline(engine::Deadline::FromDuration(std::chrono::seconds(5)));
```

## Asynchronous I/O Operations

### HTTP Client

Asynchronous HTTP requests are fundamental to microservice communication:

```cpp
#include <userver/clients/http/client.hpp>

auto response = http_client->CreateRequest()
    .get("https://api.example.com/data")
    .timeout(std::chrono::seconds(5))
    .retry(3)
    .perform();

auto data = response->body();
```

### Database Operations

Database operations are also asynchronous:

```cpp
#include <userver/storages/postgres/query.hpp>

auto result = pg_cluster->Execute(
    storages::postgres::ClusterHostType::kMaster,
    "SELECT * FROM users WHERE id = $1",
    user_id
);
```

### File I/O

File operations can be performed asynchronously:

```cpp
#include <userver/fs/read.hpp>

auto content = fs::ReadFileContents("config.json");
```

## Error Handling

### Exception Propagation

Exceptions in coroutines propagate normally:

```cpp
try {
    co_await RiskyOperation();
} catch (const std::exception& ex) {
    // Handle exception
    LOG_ERROR() << "Operation failed: " << ex.what();
    throw; // Re-throw if needed
}
```

### Cancellation

Coroutines support cooperative cancellation:

```cpp
void CancellableOperation() {
    while (!engine::current_task::IsCancelRequested()) {
        // Do work
        engine::Yield(); // Check for cancellation
    }
    
    if (engine::current_task::IsCancelRequested()) {
        throw engine::TaskCancelledException("Operation cancelled");
    }
}
```

## Best Practices

### Performance Optimization

#### Minimize Context Switches
```cpp
// Good: Batch operations
auto batch_result = database->ExecuteBatch(queries);

// Avoid: Multiple individual operations
for (const auto& query : queries) {
    database->Execute(query); // Multiple context switches
}
```

#### Efficient Resource Usage
```cpp
// Use connection pooling
auto connection = pool->Acquire();

// Return connection to pool when done
// (Automatic with RAII)
```

#### Deadline Management
```cpp
// Propagate deadlines properly
auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(30));
co_await OperationWithDeadline(deadline);
```

### Code Organization

#### Separation of Concerns
```cpp
// Separate I/O operations from business logic
class UserService {
public:
    User GetUser(UserId id) {
        auto user_data = co_await LoadUserData(id);
        return ProcessUserData(user_data);
    }
    
private:
    engine::TaskWithResult<UserData> LoadUserData(UserId id);
    User ProcessUserData(const UserData& data);
};
```

#### Error Handling Patterns
```cpp
// Handle different error types appropriately
try {
    auto result = co_await ExternalServiceCall();
    return ProcessResult(result);
} catch (const clients::http::HttpException& ex) {
    // Handle HTTP errors
    LOG_WARNING() << "HTTP error: " << ex.what();
    return DefaultResult();
} catch (const std::exception& ex) {
    // Handle other errors
    LOG_ERROR() << "Unexpected error: " << ex.what();
    throw; // Re-throw for higher-level handling
}
```

## Advanced Patterns

### Async/Await Pattern
```cpp
engine::TaskWithResult<std::vector<User>> LoadUsersAsync(
    const std::vector<UserId>& user_ids) {
    
    std::vector<engine::TaskWithResult<User>> tasks;
    
    // Start all operations concurrently
    for (const auto& id : user_ids) {
        tasks.push_back(LoadUserAsync(id));
    }
    
    // Wait for all results
    std::vector<User> users;
    for (auto& task : tasks) {
        users.push_back(task.Get());
    }
    
    co_return users;
}
```

### Timeout Handling
```cpp
template<typename T>
engine::TaskWithResult<T> WithTimeout(
    engine::TaskWithResult<T> task,
    std::chrono::milliseconds timeout) {
    
    auto deadline = engine::Deadline::FromDuration(timeout);
    
    co_await engine::WaitAny(
        task.AsTaskBase(),
        engine::SleepFor(timeout)
    );
    
    if (task.IsFinished()) {
        co_return task.Get();
    } else {
        throw std::runtime_error("Operation timed out");
    }
}
```

### Retry Logic
```cpp
template<typename Func>
auto WithRetry(Func&& func, int max_retries = 3) {
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        try {
            co_return co_await func();
        } catch (const std::exception& ex) {
            if (attempt == max_retries - 1) {
                throw; // Last attempt, re-throw
            }
            
            // Exponential backoff
            auto delay = std::chrono::milliseconds(100 * (1 << attempt));
            co_await engine::SleepFor(delay);
        }
    }
}
```

## Testing Asynchronous Code

### Unit Testing
```cpp
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_processor.hpp>

TEST_F(AsyncTest, TestAsyncFunction) {
    auto task = MyAsyncFunction();
    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(task.IsFinished());
}
```

### Integration Testing
```cpp
// Use testsuite for integration testing
// testsuite handles async operations automatically
```

## Debugging and Profiling

### Logging with Context
```cpp
// Include task ID in logs
LOG_INFO() << "Processing request " << engine::current_task::GetTaskId();

// Add context to log messages
logging::LogExtra log_extra;
log_extra.Extend("user_id", user_id.ToString());
LOG_INFO(log_extra) << "User operation completed";
```

### Performance Monitoring
```cpp
// Use metrics to monitor async operations
auto start_time = std::chrono::steady_clock::now();
co_await AsyncOperation();
auto duration = std::chrono::steady_clock::now() - start_time;

metrics_.OperationDuration->Account(duration);
```

This guide provides a comprehensive overview of asynchronous programming in userver, covering the core concepts, best practices, and advanced patterns needed to build high-performance services.