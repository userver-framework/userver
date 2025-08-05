# Userver Troubleshooting Guide

## Overview

This guide provides systematic approaches to diagnosing and resolving common issues encountered when working with the userver framework. It covers debugging techniques, performance optimization, and solutions to frequently encountered problems.

## Debugging Techniques

### Logging and Monitoring

#### Structured Logging
```cpp
// Use structured logging with context
logging::LogExtra log_extra;
log_extra.Extend("user_id", user_id.ToString());
log_extra.Extend("request_id", request_id.ToString());

LOG_INFO(log_extra) << "Processing user request";
```

#### Log Levels
```cpp
// Trace - Very detailed information, typically of interest only when diagnosing problems
LOG_TRACE() << "Detailed function entry";

// Debug - Information useful for debugging
LOG_DEBUG() << "Variable value: " << variable;

// Info - General information about program execution
LOG_INFO() << "Service started successfully";

// Warning - Potentially harmful situations
LOG_WARNING() << "Deprecated API usage detected";

// Error - Error events that might still allow the application to continue running
LOG_ERROR() << "Failed to process request: " << error.what();

// Critical - Very severe error events that might lead to application termination
LOG_CRITICAL() << "Service initialization failed";
```

### Debugging Tools

#### GDB Debugging
```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Run service under GDB
gdb --args ./service --config config.yaml

# Useful GDB commands
(gdb) break MyComponent::MyMethod
(gdb) run
(gdb) bt  # backtrace
(gdb) info locals
(gdb) continue
```

#### Core Dumps
```bash
# Enable core dumps
ulimit -c unlimited

# Analyze core dump
gdb ./service core.dump
(gdb) bt
(gdb) info threads
```

### Runtime Diagnostics

#### Health Check Endpoints
```cpp
class HealthCheckHandler : public server::handlers::HttpHandlerBase {
public:
    std::string HandleRequestThrow(
        const server::http::HttpRequest& request,
        server::request::RequestContext& context) const override {
        
        formats::json::ValueBuilder response;
        
        // Check component health
        response["status"] = "ok";
        response["components"] = CheckComponentHealth();
        response["metrics"] = GetCurrentMetrics();
        
        return response.ExtractValue().ToString();
    }
    
private:
    formats::json::Value CheckComponentHealth() const {
        formats::json::ValueBuilder components;
        
        // Check database connectivity
        components["database"] = database_->IsConnected() ? "ok" : "error";
        
        // Check cache status
        components["cache"] = cache_->IsHealthy() ? "ok" : "error";
        
        return components.ExtractValue();
    }
};
```

#### Debug Endpoints
```cpp
class DebugHandler : public server::handlers::HttpHandlerBase {
public:
    std::string HandleRequestThrow(
        const server::http::HttpRequest& request,
        server::request::RequestContext& context) const override {
        
        auto action = request.GetArg("action");
        
        if (action == "dump-config") {
            return DumpConfiguration();
        } else if (action == "dump-cache") {
            return DumpCacheContents();
        } else if (action == "force-gc") {
            engine::ForceGarbageCollection();
            return "Garbage collection triggered";
        }
        
        throw server::handlers::ClientError(
            server::handlers::ExternalBody{"Unknown action"}
        );
    }
};
```

## Performance Issues

### Common Performance Problems

#### High Latency
```cpp
// Use metrics to identify bottlenecks
class LatencyTracker {
public:
    LatencyTracker(const std::string& operation_name)
        : start_time_(std::chrono::steady_clock::now())
        , operation_name_(operation_name) {}
    
    ~LatencyTracker() {
        auto duration = std::chrono::steady_clock::now() - start_time_;
        metrics_->RecordLatency(operation_name_, duration);
    }
    
private:
    std::chrono::steady_clock::time_point start_time_;
    std::string operation_name_;
};
```

#### Resource Exhaustion
```cpp
// Monitor resource usage
void CheckResourceLimits() {
    auto memory_usage = GetMemoryUsage();
    auto cpu_usage = GetCpuUsage();
    auto connection_count = GetActiveConnections();
    
    if (memory_usage > kMemoryThreshold) {
        LOG_WARNING() << "High memory usage: " << memory_usage;
    }
    
    if (connection_count > kConnectionThreshold) {
        LOG_WARNING() << "High connection count: " << connection_count;
    }
}
```

### Profiling Tools

#### CPU Profiling with perf
```bash
# Profile running service
perf record -g -p $(pgrep service)

# Analyze results
perf report

# Generate flame graph
perf script | flamegraph.pl > flame.svg
```

#### Memory Profiling with Valgrind
```bash
# Memory leak detection
valgrind --leak-check=full --show-leak-kinds=all ./service

# Heap profiling
valgrind --tool=massif ./service
ms_print massif.out.*
```

### Optimization Strategies

#### Connection Pooling
```cpp
// Configure appropriate pool sizes
yaml_config::YamlConfig config = R"(
http_client:
  pools:
    default:
      max_connections: 100
      max_connections_per_host: 20
      queue_size: 50
)";

// Monitor pool usage
auto pool_stats = http_client_->GetPoolStatistics();
LOG_DEBUG() << "Pool usage: " << pool_stats.active_connections 
            << "/" << pool_stats.max_connections;
```

#### Caching Strategies
```cpp
// Implement cache warming
void WarmCache() {
    // Pre-load frequently accessed data
    auto popular_users = LoadPopularUsers();
    for (const auto& user : popular_users) {
        user_cache_->Put(user.id, user);
    }
}

// Monitor cache performance
void LogCacheMetrics() {
    auto stats = user_cache_->GetStatistics();
    LOG_INFO() << "Cache hit rate: " << stats.hit_rate
               << ", size: " << stats.size;
}
```

## Common Errors and Solutions

### Configuration Errors

#### Missing Component Dependencies
```yaml
# Error: Component 'my-component' depends on missing component 'database'
components_manager:
  components:
    my-component:
      dependencies:
        - database  # Make sure this component is defined
```

**Solution**: Ensure all dependencies are defined in the configuration file.

#### Invalid Configuration Values
```cpp
// Handle configuration validation
void ValidateConfig(const MyComponentConfig& config) {
    if (config.timeout.count() <= 0) {
        throw std::runtime_error("Timeout must be positive");
    }
    
    if (config.max_retries < 0) {
        throw std::runtime_error("Max retries cannot be negative");
    }
}
```

### Database Issues

#### Connection Failures
```cpp
// Implement connection retry logic
auto result = utils::RetryUntilOk(
    [this]() { return database_->Execute(query); },
    std::chrono::seconds(30),  // timeout
    std::chrono::milliseconds(100)  // initial delay
);
```

#### Query Performance
```cpp
// Use EXPLAIN to analyze queries
auto explain_result = database_->Execute(
    "EXPLAIN (ANALYZE, BUFFERS) " + query.GetQuery()
);

LOG_DEBUG() << "Query plan: " << explain_result.AsSingleRow<std::string>();
```

### HTTP Client Issues

#### Timeout Handling
```cpp
try {
    auto response = http_client_->CreateRequest()
        .get(url)
        .timeout(std::chrono::seconds(5))
        .retry(3)
        .perform();
        
    if (response->IsOk()) {
        return response->body();
    }
} catch (const clients::http::TimeoutException& ex) {
    LOG_WARNING() << "Request timeout: " << ex.what();
    // Implement fallback logic
    return GetCachedResponse();
}
```

#### Circuit Breaker Pattern
```cpp
class CircuitBreakerHttpClient {
public:
    std::unique_ptr<clients::http::Response> PerformRequest(
        clients::http::RequestBuilder&& request) {
        
        if (IsOpen()) {
            throw std::runtime_error("Circuit breaker is open");
        }
        
        try {
            auto response = client_->Perform(std::move(request));
            OnSuccess();
            return response;
        } catch (const std::exception& ex) {
            OnFailure();
            throw;
        }
    }
    
private:
    bool IsOpen() const {
        return failure_count_ > failure_threshold_ &&
               std::chrono::steady_clock::now() < next_retry_time_;
    }
    
    void OnSuccess() {
        failure_count_ = 0;
        state_ = State::kClosed;
    }
    
    void OnFailure() {
        ++failure_count_;
        if (failure_count_ >= failure_threshold_) {
            state_ = State::kOpen;
            next_retry_time_ = std::chrono::steady_clock::now() + timeout_;
        }
    }
};
```

### Concurrency Issues

#### Race Conditions
```cpp
// Use proper synchronization primitives
class ThreadSafeCounter {
public:
    void Increment() {
        std::lock_guard lock(mutex_);
        ++value_;
    }
    
    int GetValue() const {
        std::lock_guard lock(mutex_);
        return value_;
    }
    
private:
    mutable std::mutex mutex_;
    int value_{0};
};
```

#### Deadlock Prevention
```cpp
// Always acquire locks in consistent order
class AccountManager {
public:
    void Transfer(Account& from, Account& to, int amount) {
        // Acquire locks in consistent order to prevent deadlock
        std::lock(from.mutex_, to.mutex_);
        std::lock_guard lock1(from.mutex_, std::adopt_lock);
        std::lock_guard lock2(to.mutex_, std::adopt_lock);
        
        from.balance_ -= amount;
        to.balance_ += amount;
    }
};
```

## Testing and Validation

### Unit Test Debugging
```cpp
class MockDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_database_ = std::make_shared<MockDatabase>();
        service_ = std::make_unique<MyService>(mock_database_);
    }
    
    // Verify mock expectations
    void TearDown() override {
        ::testing::Mock::VerifyAndClearExpectations(mock_database_.get());
    }
    
    std::shared_ptr<MockDatabase> mock_database_;
    std::unique_ptr<MyService> service_;
};
```

### Integration Test Issues

#### Test Environment Setup
```python
# testsuite configuration for consistent testing
pytest_plugins = ['pytest_userver.plugins.core']

@pytest.fixture(scope='session')
def service_config_yaml():
    return {
        'components_manager': {
            'components': {
                'my-component': {
                    'some-setting': 'test-value'
                }
            }
        }
    }

# Use fixtures for test data
@pytest.fixture
def sample_user():
    return {'id': 123, 'name': 'Test User'}
```

#### Flaky Test Resolution
```python
# Use retry mechanisms for flaky tests
@pytest.mark.flaky(reruns=3)
async def test_flaky_operation(service_client):
    response = await service_client.post('/flaky-endpoint')
    assert response.status == 200

# Add proper test cleanup
@pytest.fixture(autouse=True)
def cleanup_database():
    yield
    # Cleanup after each test
    database.clear_test_data()
```

## Deployment Issues

### Startup Failures

#### Component Initialization Errors
```cpp
// Implement proper error handling in component constructors
MyComponent::MyComponent(const components::ComponentConfig& config,
                         const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
    
    try {
        InitializeResources();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to initialize component: " << ex.what();
        throw; // Re-throw to prevent service startup
    }
}
```

#### Configuration Loading Issues
```cpp
// Validate configuration early
void ValidateConfiguration(const yaml_config::YamlConfig& config) {
    if (!config.HasMember("required-setting")) {
        throw std::runtime_error("Missing required configuration: required-setting");
    }
    
    auto value = config["required-setting"].As<std::string>();
    if (value.empty()) {
        throw std::runtime_error("required-setting cannot be empty");
    }
}
```

### Runtime Issues

#### Memory Leaks
```cpp
// Use RAII for resource management
class ResourceManager {
public:
    ResourceManager() : resource_(AllocateResource()) {}
    ~ResourceManager() { FreeResource(resource_); }
    
    // Disable copy constructor and assignment
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    // Enable move constructor and assignment
    ResourceManager(ResourceManager&& other) noexcept
        : resource_(other.resource_) {
        other.resource_ = nullptr;
    }
    
private:
    void* resource_;
};
```

#### Resource Exhaustion
```cpp
// Implement resource limits
class ResourceLimiter {
public:
    bool TryAcquire() {
        std::lock_guard lock(mutex_);
        if (current_count_ < max_count_) {
            ++current_count_;
            return true;
        }
        return false;
    }
    
    void Release() {
        std::lock_guard lock(mutex_);
        --current_count_;
    }
    
private:
    std::mutex mutex_;
    int current_count_{0};
    int max_count_{100};
};
```

## Monitoring and Alerting

### Metrics Collection
```cpp
// Define custom metrics
class MyServiceMetrics final : public utils::statistics::MetricTag<MyServiceMetrics> {
public:
    const utils::statistics::MetricTag kRequestsTotal{
        "requests_total", {"Total number of requests"}
    };
    
    const utils::statistics::MetricTag kRequestDuration{
        "request_duration", {"Request duration in seconds"}
    };
};

// Record metrics
void RecordRequestMetrics(const std::string& endpoint, 
                         std::chrono::milliseconds duration) {
    metrics_->requests_total->Add(1, {{"endpoint", endpoint}});
    metrics_->request_duration->Observe(duration.count() / 1000.0);
}
```

### Health Checks
```cpp
// Implement comprehensive health checks
formats::json::Value CheckServiceHealth() {
    formats::json::ValueBuilder response;
    
    // Check database connectivity
    response["database"] = CheckDatabaseHealth();
    
    // Check external service dependencies
    response["external_services"] = CheckExternalServices();
    
    // Check resource usage
    response["resources"] = CheckResourceUsage();
    
    // Overall status
    bool healthy = response["database"].As<bool>() &&
                   response["external_services"].As<bool>();
    response["status"] = healthy ? "healthy" : "unhealthy";
    
    return response.ExtractValue();
}
```

This troubleshooting guide provides a comprehensive approach to diagnosing and resolving issues in userver-based services. By following these patterns and techniques, developers can quickly identify and resolve problems while maintaining service reliability and performance.