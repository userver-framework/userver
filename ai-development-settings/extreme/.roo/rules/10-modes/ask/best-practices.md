# Best Practices and Recommended Patterns

## Overview

Comprehensive guidance on userver best practices, recommended patterns, and proven approaches for building robust, performant, and maintainable services.

## Core Development Best Practices

### Asynchronous Programming Patterns

**Coroutine Safety Guidelines**
```cpp
// ✅ Good: Proper async operation
engine::TaskWithResult<UserData> GetUserAsync(int user_id) {
    auto result = co_await database_.ExecuteAsync(
        "SELECT * FROM users WHERE id = $1", user_id
    );
    co_return ParseUserData(result);
}

// ❌ Avoid: Blocking operations in main task processor
UserData GetUserBlocking(int user_id) {
    // This blocks the entire task processor!
    auto result = database_.ExecuteBlocking(query);
    return ParseUserData(result);
}
```

**Concurrent Operations**
```cpp
// ✅ Good: Concurrent async operations
engine::TaskWithResult<CombinedData> FetchCombinedData(int user_id) {
    auto user_task = utils::Async("fetch_user", [this, user_id]() {
        return GetUserAsync(user_id);
    });
    
    auto settings_task = utils::Async("fetch_settings", [this, user_id]() {
        return GetUserSettingsAsync(user_id);
    });
    
    auto user = co_await user_task;
    auto settings = co_await settings_task;
    
    co_return CombineData(user, settings);
}
```

**Lifetime Management**
```cpp
// ✅ Good: Capture by value for async operations
engine::TaskWithResult<void> ProcessDataSafely(std::string data) {
    auto task = utils::Async("process", [data = std::move(data)]() {
        return ProcessData(data);  // Safe: data is owned by lambda
    });
    co_await task;
}

// ❌ Avoid: Capture by reference with potential lifetime issues
engine::TaskWithResult<void> ProcessDataUnsafely(const std::string& data) {
    auto task = utils::Async("process", [&data]() {
        return ProcessData(data);  // Dangerous: data might be destroyed
    });
    co_await task;  // data reference might be invalid
}
```

### Component Design Patterns

**Component Initialization**
```cpp
class DatabaseComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "database";
    
    DatabaseComponent(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
        : ComponentBase(config, context),
          pg_cluster_(context.FindComponent<components::Postgres>().GetCluster()) {
        // Initialize component resources
        InitializeConnectionPool();
    }
    
    ~DatabaseComponent() override {
        // Cleanup is automatic with RAII
    }
    
private:
    storages::postgres::ClusterPtr pg_cluster_;
};
```

**Handler Best Practices**
```cpp
class UserHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user";
    
    UserHandler(const components::ComponentConfig& config,
               const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          database_(context.FindComponent<DatabaseComponent>()) {}
    
    std::string HandleRequest(server::http::HttpRequest& request,
                            server::request::RequestContext& ctx) const override {
        // Set appropriate content type
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        
        // Validate input early
        auto user_id = request.GetPathArg("user_id");
        if (user_id.empty()) {
            throw server::handlers::ClientError(400, "Missing user_id");
        }
        
        // Use structured logging
        LOG_INFO() << "Processing user request"
                   << logging::LogExtra::Key("user_id", user_id)
                   << logging::LogExtra::Key("handler", kName);
        
        try {
            auto user_data = database_.GetUser(std::stoi(user_id));
            return SerializeUserData(user_data);
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to get user data: " << e.what()
                       << logging::LogExtra::Key("user_id", user_id);
            throw server::handlers::InternalServerError();
        }
    }
    
private:
    DatabaseComponent& database_;
};
```

### Error Handling Best Practices

**Exception Hierarchy**
```cpp
// Custom exception hierarchy for different error types
class ServiceError : public std::runtime_error {
public:
    explicit ServiceError(const std::string& message) 
        : std::runtime_error(message) {}
};

class ValidationError : public ServiceError {
public:
    explicit ValidationError(const std::string& field)
        : ServiceError("Validation failed for field: " + field) {}
};

class DatabaseError : public ServiceError {
public:
    explicit DatabaseError(const std::string& operation)
        : ServiceError("Database operation failed: " + operation) {}
};
```

**Handler Error Handling**
```cpp
std::string HandleRequest(server::http::HttpRequest& request,
                         server::request::RequestContext&) const override {
    try {
        auto data = ValidateAndParseRequest(request);
        auto result = ProcessBusinessLogic(data);
        return SerializeResponse(result);
        
    } catch (const ValidationError& e) {
        LOG_WARNING() << "Validation error: " << e.what();
        throw server::handlers::ClientError(400, e.what());
        
    } catch (const DatabaseError& e) {
        LOG_ERROR() << "Database error: " << e.what();
        throw server::handlers::InternalServerError();
        
    } catch (const std::exception& e) {
        LOG_ERROR() << "Unexpected error: " << e.what();
        throw server::handlers::InternalServerError();
    }
}
```

### Configuration Management

**Static Configuration Best Practices**
```yaml
# Production-ready static configuration
components_manager:
  task_processors:
    main-task-processor:
      worker_threads: 8  # Match CPU cores
      thread_name: main-worker
      
    fs-task-processor:
      worker_threads: 2  # Minimal for file operations
      thread_name: fs-worker
      
  components:
    server:
      listener:
        port: 8080
        max_connections: 32768
        
    logging:
      loggers:
        default:
          file_path: '@stdout'  # Use stdout for containerized environments
          level: info
          format: tskv  # Structured logging format
          
    http-client:
      pool-statistics-disable: false
      thread-name-prefix: http-client
      destination-metrics-auto-max-size: 100
```

**Dynamic Configuration Usage**
```cpp
class ConfigurableService {
    dynamic_config::Source config_source_;
    
public:
    void ProcessRequest() {
        auto config = config_source_.GetSnapshot();
        
        // Get timeout from dynamic config
        auto timeout = config[kRequestTimeout];
        auto max_retries = config[kMaxRetries];
        
        // Use configuration values
        ProcessWithTimeout(timeout, max_retries);
    }
};
```

### Database Integration Patterns

**Connection Pool Management**
```cpp
class DatabaseService {
    storages::postgres::ClusterPtr pg_cluster_;
    
public:
    // Use appropriate host type for read/write operations
    engine::TaskWithResult<UserData> GetUser(int user_id) {
        auto result = co_await pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kSlave,  // Read from replica
            "SELECT * FROM users WHERE id = $1", user_id
        );
        co_return ParseUserData(result);
    }
    
    engine::TaskWithResult<void> UpdateUser(const UserData& user) {
        auto result = co_await pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,  // Write to primary
            "UPDATE users SET name = $1, email = $2 WHERE id = $3",
            user.name, user.email, user.id
        );
    }
};
```

**Transaction Management**
```cpp
engine::TaskWithResult<void> TransferFunds(int from_user, int to_user, 
                                          decimal64::Decimal amount) {
    auto transaction = co_await pg_cluster_->Begin(
        storages::postgres::ClusterHostType::kMaster,
        storages::postgres::TransactionOptions{
            .isolation_level = storages::postgres::IsolationLevel::kSerializable
        }
    );
    
    try {
        // Debit from source account
        co_await transaction.Execute(
            "UPDATE accounts SET balance = balance - $1 WHERE user_id = $2",
            amount, from_user
        );
        
        // Credit to destination account
        co_await transaction.Execute(
            "UPDATE accounts SET balance = balance + $1 WHERE user_id = $2",
            amount, to_user
        );
        
        co_await transaction.Commit();
        
    } catch (const std::exception& e) {
        co_await transaction.Rollback();
        throw;
    }
}
```

### Performance Optimization Patterns

**Efficient Resource Usage**
```cpp
class OptimizedService {
    // Use object pools for frequently allocated objects
    utils::ObjectPool<ExpensiveObject> object_pool_;
    
    // Cache frequently accessed data
    cache::LruCache<std::string, CachedData> data_cache_;
    
public:
    engine::TaskWithResult<ProcessedData> ProcessRequest(const Request& req) {
        // Reuse pooled objects
        auto obj = object_pool_.Acquire();
        
        // Check cache first
        if (auto cached = data_cache_.Get(req.key)) {
            co_return ProcessCachedData(*cached, *obj);
        }
        
        // Fetch and cache data
        auto data = co_await FetchData(req.key);
        data_cache_.Put(req.key, data);
        
        co_return ProcessFreshData(data, *obj);
    }
};
```

**Batch Operations**
```cpp
// ✅ Good: Batch database operations
engine::TaskWithResult<void> UpdateMultipleUsers(
    const std::vector<UserUpdate>& updates) {
    
    if (updates.empty()) co_return;
    
    // Build batch query
    std::string query = "UPDATE users SET name = data.name, email = data.email "
                       "FROM (VALUES ";
    
    std::vector<storages::postgres::ParameterStore> params;
    for (size_t i = 0; i < updates.size(); ++i) {
        if (i > 0) query += ", ";
        query += fmt::format("(${}, ${}, ${})", 
                           i * 3 + 1, i * 3 + 2, i * 3 + 3);
        params.emplace_back(updates[i].name, updates[i].email, updates[i].id);
    }
    
    query += ") AS data(name, email, id) WHERE users.id = data.id";
    
    co_await pg_cluster_->Execute(
        storages::postgres::ClusterHostType::kMaster,
        query, params
    );
}
```

### Security Best Practices

**Input Validation and Sanitization**
```cpp
class SecureHandler : public server::handlers::HttpHandlerBase {
    std::string HandleRequest(server::http::HttpRequest& request,
                            server::request::RequestContext&) const override {
        // Validate content length
        if (request.GetBody().size() > MAX_REQUEST_SIZE) {
            throw server::handlers::ClientError(413, "Request too large");
        }
        
        // Validate content type
        auto content_type = request.GetHeader("Content-Type");
        if (content_type != "application/json") {
            throw server::handlers::ClientError(415, "Unsupported media type");
        }
        
        // Parse and validate JSON
        auto json_data = ValidateAndParseJson(request.GetBody());
        
        // Sanitize string inputs
        auto user_input = SanitizeString(json_data["user_input"].As<std::string>());
        
        return ProcessSecureInput(user_input);
    }
    
private:
    std::string SanitizeString(const std::string& input) {
        // Remove potentially dangerous characters
        std::string sanitized;
        std::copy_if(input.begin(), input.end(), std::back_inserter(sanitized),
                    [](char c) { return std::isalnum(c) || std::isspace(c); });
        return sanitized;
    }
};
```

**Authentication and Authorization**
```cpp
class AuthenticatedHandler : public server::handlers::HttpHandlerBase {
    std::string HandleRequest(server::http::HttpRequest& request,
                            server::request::RequestContext& ctx) const override {
        // Extract and validate authentication token
        auto auth_header = request.GetHeader("Authorization");
        if (!auth_header.starts_with("Bearer ")) {
            throw server::handlers::Unauthorized();
        }
        
        auto token = auth_header.substr(7);  // Remove "Bearer " prefix
        auto user_info = ValidateToken(token);
        
        // Check authorization for specific resource
        auto resource_id = request.GetPathArg("resource_id");
        if (!user_info.HasAccessTo(resource_id)) {
            throw server::handlers::Forbidden();
        }
        
        // Store user context for downstream processing
        ctx.SetData("user_info", user_info);
        
        return ProcessAuthorizedRequest(request, ctx);
    }
};
```

### Monitoring and Observability

**Structured Logging Patterns**
```cpp
class ObservableService {
public:
    engine::TaskWithResult<ProcessResult> ProcessData(const RequestData& data) {
        auto start_time = std::chrono::steady_clock::now();
        
        LOG_INFO() << "Starting data processing"
                   << logging::LogExtra::Key("request_id", data.request_id)
                   << logging::LogExtra::Key("data_size", data.payload.size())
                   << logging::LogExtra::Key("operation", "process_data");
        
        try {
            auto result = co_await DoProcessing(data);
            
            auto duration = std::chrono::steady_clock::now() - start_time;
            LOG_INFO() << "Data processing completed"
                       << logging::LogExtra::Key("request_id", data.request_id)
                       << logging::LogExtra::Key("duration_ms", 
                                                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count())
                       << logging::LogExtra::Key("result_size", result.size());
            
            co_return result;
            
        } catch (const std::exception& e) {
            LOG_ERROR() << "Data processing failed"
                       << logging::LogExtra::Key("request_id", data.request_id)
                       << logging::LogExtra::Key("error", e.what())
                       << logging::LogExtra::Stacktrace();
            throw;
        }
    }
};
```

**Custom Metrics**
```cpp
class MetricsCollector {
    utils::statistics::Counter requests_total_;
    utils::statistics::Counter requests_failed_;
    utils::statistics::Histogram request_duration_;
    utils::statistics::Gauge active_connections_;
    
public:
    void RecordRequest(std::chrono::milliseconds duration, bool success) {
        requests_total_.Increment();
        if (!success) {
            requests_failed_.Increment();
        }
        request_duration_.Account(duration.count());
    }
    
    void UpdateActiveConnections(int count) {
        active_connections_.Set(count);
    }
};
```

**Distributed Tracing**
```cpp
class TracedService {
public:
    engine::TaskWithResult<ServiceResult> ProcessRequest(const Request& req) {
        tracing::Span span("process_request");
        span.AddTag("request_type", req.type);
        span.AddTag("user_id", req.user_id);
        
        {
            tracing::Span db_span("database_query");
            auto data = co_await database_.FetchData(req.data_id);
            db_span.AddTag("rows_fetched", data.size());
        }
        
        {
            tracing::Span processing_span("business_logic");
            auto result = ProcessBusinessLogic(data);
            processing_span.AddTag("processing_time_ms", 
                                  processing_span.GetElapsedTime().count());
        }
        
        span.AddTag("result_status", "success");
        co_return result;
    }
};
```

## Testing Best Practices

### Unit Testing Patterns

**Component Testing**
```cpp
#include <userver/utest/utest.hpp>

class DatabaseServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test database connection
        service_ = std::make_unique<DatabaseService>(test_pg_cluster_);
    }
    
    std::unique_ptr<DatabaseService> service_;
    storages::postgres::ClusterPtr test_pg_cluster_;
};

UTEST_F(DatabaseServiceTest, GetUser_ValidId_ReturnsUser) {
    // Arrange
    const int user_id = 123;
    
    // Act
    auto user = service_->GetUser(user_id).Get();
    
    // Assert
    EXPECT_EQ(user.id, user_id);
    EXPECT_FALSE(user.name.empty());
}
```

**Handler Testing**
```cpp
UTEST(UserHandler, HandleRequest_ValidInput_ReturnsUserData) {
    // Create test components
    auto component_list = components::MinimalServerComponentList()
        .Append<DatabaseComponent>()
        .Append<UserHandler>();
    
    // Set up test request
    auto request = server::http::HttpRequestBuilder()
        .SetMethod(server::http::HttpMethod::kGet)
        .SetUrl("/users/123")
        .Build();
    
    // Execute handler
    UserHandler handler(/* config */, /* context */);
    auto response = handler.HandleRequest(request, /* context */);
    
    // Verify response
    EXPECT_TRUE(response.find("\"id\":123") != std::string::npos);
}
```

### Integration Testing

**Service Testing with TestSuite**
```python
# testsuite functional test
async def test_user_endpoint(service_client):
    # Test successful user retrieval
    response = await service_client.get('/users/123')
    assert response.status == 200
    
    user_data = response.json()
    assert user_data['id'] == 123
    assert 'name' in user_data
    
    # Test error handling
    response = await service_client.get('/users/invalid')
    assert response.status == 400
```

## Deployment and Production Patterns

### Configuration Management
```yaml
# Production configuration template
components_manager:
  task_processors:
    main-task-processor:
      worker_threads: 16  # Scale with available CPU cores
      
  components:
    server:
      listener:
        port: 8080
        max_connections: 65536
        
    logging:
      loggers:
        default:
          file_path: '@stdout'
          level: info
          format: tskv
          
    postgres-db:
      dbalias: main_db
      blocking_task_processor: fs-task-processor
      dns_resolver: async
      
    dynamic-config:
      fs-cache-path: /var/cache/service/dynamic-config.json
      fs-task-processor: fs-task-processor
```

### Health Checks and Monitoring
```cpp
class HealthCheckHandler : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-health";
    
    std::string HandleRequest(server::http::HttpRequest& request,
                            server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        
        // Check critical dependencies
        bool db_healthy = CheckDatabaseHealth();
        bool cache_healthy = CheckCacheHealth();
        
        if (db_healthy && cache_healthy) {
            return R"({"status":"healthy","timestamp":")" + 
                   utils::datetime::Timestring() + "\"}";
        } else {
            request.GetHttpResponse().SetStatus(server::http::HttpStatus::kServiceUnavailable);
            return R"({"status":"unhealthy","database":)" + 
                   (db_healthy ? "true" : "false") + 
                   R"(,"cache":)" + (cache_healthy ? "true" : "false") + "}";
        }
    }
};
```

## Anti-Patterns to Avoid

### Common Mistakes

**Blocking Operations in Main Task Processor**
```cpp
// ❌ Wrong: Blocking file I/O in main task processor
std::string ReadConfigFile() {
    std::ifstream file("config.txt");  // Blocks entire task processor!
    std::string content;
    // ... read file
    return content;
}

// ✅ Correct: Use fs-task-processor for file operations
engine::TaskWithResult<std::string> ReadConfigFileAsync() {
    co_return co_await utils::Async("read_config", 
                                   utils::TaskProcessor::Get("fs-task-processor"),
                                   []() {
        std::ifstream file("config.txt");
        std::string content;
        // ... read file
        return content;
    });
}
```

**Improper Exception Handling**
```cpp
// ❌ Wrong: Swallowing exceptions
void ProcessData() {
    try {
        DoSomethingRisky();
    } catch (...) {
        // Silent failure - very bad!
    }
}

// ✅ Correct: Proper exception handling
void ProcessData() {
    try {
        DoSomethingRisky();
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to process data: " << e.what();
        throw;  // Re-throw or handle appropriately
    }
}
```

**Resource Leaks**
```cpp
// ❌ Wrong: Manual resource management
class BadResourceManager {
    Resource* resource_;
public:
    BadResourceManager() : resource_(new Resource()) {}
    // Missing destructor - resource leak!
};

// ✅ Correct: RAII resource management
class GoodResourceManager {
    std::unique_ptr<Resource> resource_;
public:
    GoodResourceManager() : resource_(std::make_unique<Resource>()) {}
    // Automatic cleanup with unique_ptr
};
```

## Performance Guidelines

### Memory Management
- Use RAII for automatic resource cleanup
- Prefer stack allocation over heap when possible
- Use object pools for frequently allocated objects
- Monitor memory usage with built-in profiling tools

### Concurrency Optimization
- Maximize async operation concurrency
- Use appropriate task processors for different workloads
- Avoid unnecessary synchronization
- Batch operations when possible

### Database Optimization
- Use connection pooling effectively
- Choose appropriate host types (master/slave)
- Implement proper transaction boundaries
- Monitor connection pool metrics

### Caching Strategies
- Cache frequently accessed data
- Use appropriate cache eviction policies
- Monitor cache hit rates
- Implement cache warming for critical data

## Quality Assurance

### Code Review Checklist
- [ ] Async operations use proper coroutine patterns
- [ ] Error handling is comprehensive and appropriate
- [ ] Resources are managed with RAII
- [ ] Logging includes sufficient context
- [ ] Configuration is externalized
- [ ] Tests cover critical functionality
- [ ] Security considerations are addressed
- [ ] Performance implications are considered

### Production Readiness
- [ ] Health checks implemented
- [ ] Metrics and monitoring configured
- [ ] Graceful shutdown handling
- [ ] Resource limits configured
- [ ] Security hardening applied
- [ ] Documentation updated
- [ ] Deployment procedures tested
- [ ] Rollback procedures defined