# Framework Fundamentals - Global Rules

**Rule ID**: `global.framework.fundamentals`  
**Priority**: 1000  
**Scope**: framework-wide  
**Override**: false  

## Core Principles

### Asynchronous-First Architecture
userver is built around coroutines and asynchronous operations:

```cpp
// ✅ Correct: Asynchronous database operation
auto result = co_await pg_cluster->Execute(
    userver::storages::postgres::ClusterHostType::kMaster,
    "SELECT * FROM users WHERE id = $1", user_id
);

// ❌ Incorrect: Blocking operation in coroutine context
std::this_thread::sleep_for(std::chrono::seconds(1));
```

**Memory Bank Reference**: [`memory-bank://main/async-programming`](../memory-bank/main/async-programming.md)

### Component-Based Architecture
All functionality is organized into components with clear lifecycle:

```cpp
class MyComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "my-component";
    
    MyComponent(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);
    
    // Component lifecycle is managed automatically
    ~MyComponent() override = default;
};
```

**Memory Bank Reference**: [`memory-bank://main/component-system`](../memory-bank/main/component-system.md)

### Exception-Safe Error Handling
Use exceptions for error handling - they're automatically converted to HTTP responses:

```cpp
std::string HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const override {
    
    auto user_id = request.GetArg("user_id");
    if (user_id.empty()) {
        throw userver::server::handlers::ClientError(
            userver::server::handlers::ExternalBody{"Missing user_id parameter"}
        );
    }
    
    return ProcessUser(user_id);
}
```

**Memory Bank Reference**: [`memory-bank://main/framework-core#error-handling`](../memory-bank/main/framework-core.md#error-handling)

## Mandatory Patterns

### Service Structure
Every userver service must follow this structure:

```
service_name/
├── CMakeLists.txt              # Build configuration
├── configs/
│   └── static_config.yaml      # Service configuration
├── src/
│   ├── main.cpp               # Service entry point
│   └── handlers/              # HTTP handlers
└── tests/                     # Test files
```

**Cross-Reference**: [`pattern://service_structure/standard_layout`](../memory-bank/main/service-patterns.md#service-structure)

### Configuration Management
All services must support both static and dynamic configuration:

```yaml
# static_config.yaml
components:
    handler-hello:
        path: /hello
        method: GET,POST
        task_processor: main-task-processor
    
    dynamic-config:
        fs-cache-path: /var/cache/service/dynamic_config_cache.json
```

**Memory Bank Reference**: [`memory-bank://main/framework-core#configuration`](../memory-bank/main/framework-core.md#configuration)

### Task Processor Configuration
Services must define appropriate task processors:

```yaml
task_processors:
    main-task-processor:
        worker_threads: 4          # CPU-bound tasks
    fs-task-processor:
        worker_threads: 2          # File system operations
    monitor-task-processor:
        worker_threads: 1          # Monitoring tasks
```

**Cross-Reference**: [`concept://concurrency/task_processors`](../memory-bank/main/async-programming.md#task-processors)

## Non-Negotiable Requirements

### Coroutine Safety
- **NEVER** use blocking operations in coroutine context
- **ALWAYS** use userver's async drivers for I/O
- **ALWAYS** propagate cancellation and deadlines

```cpp
// ✅ Correct: Using userver's async HTTP client
auto response = co_await http_client.CreateRequest()
    .post(url, data)
    .timeout(std::chrono::seconds(5))
    .perform();

// ❌ Incorrect: Using blocking HTTP library
auto response = blocking_http_lib::post(url, data);
```

### Memory Management
- **ALWAYS** use RAII patterns
- **PREFER** smart pointers over raw pointers
- **AVOID** manual memory management

### Component Dependencies
- **DECLARE** all dependencies in constructor
- **USE** ComponentContext to access other components
- **FOLLOW** dependency injection patterns

```cpp
MyHandler(const userver::components::ComponentConfig& config,
          const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<userver::components::Postgres>("postgres-db")),
      redis_client_(context.FindComponent<userver::components::Redis>("redis")) {
}
```

## Framework Integration Points

### Logging Integration
Use structured logging with correlation IDs:

```cpp
LOG_INFO() << "Processing request" 
           << userver::logging::LogExtra{{"user_id", user_id}, 
                                        {"operation", "get_profile"}};
```

**Memory Bank Reference**: [`memory-bank://main/framework-core#logging`](../memory-bank/main/framework-core.md#logging)

### Metrics Integration
Expose metrics for monitoring:

```cpp
auto& metrics_storage = context.FindComponent<userver::components::StatisticsStorage>();
auto counter = metrics_storage.GetMetric("my_service.requests_total");
counter.Inc();
```

**Cross-Reference**: [`pattern://monitoring/metrics_collection`](../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md)

### Health Checks
Always include health check endpoints:

```cpp
auto component_list = userver::components::MinimalServerComponentList()
    .Append<userver::server::handlers::Ping>()
    .Append<userver::server::handlers::TestsControl>()
    .Append<MyHandler>();
```

## Quality Standards

### Code Quality
- **Platinum Tier**: Comprehensive testing, documentation, monitoring
- **Golden Tier**: Good testing coverage, basic documentation
- **Silver Tier**: Basic testing, minimal documentation

### Performance Requirements
- **Response Time**: < 100ms for simple operations
- **Throughput**: Handle expected load with 2x safety margin
- **Resource Usage**: Efficient memory and CPU utilization

### Reliability Standards
- **Availability**: 99.9% uptime minimum
- **Error Handling**: Graceful degradation
- **Recovery**: Automatic recovery from transient failures

## Cross-References

### Related Memory Bank Entries
- [`memory-bank://main/framework-core`](../memory-bank/main/framework-core.md) - Core framework concepts
- [`memory-bank://main/component-system`](../memory-bank/main/component-system.md) - Component architecture
- [`memory-bank://main/async-programming`](../memory-bank/main/async-programming.md) - Async patterns
- [`memory-bank://main/service-patterns`](../memory-bank/main/service-patterns.md) - Service design patterns

### Implementation Examples
- [`example://basic_service/hello_world`](../../samples/hello_service/) - Basic service implementation
- [`example://database_service/postgres`](../../samples/postgres_service/) - Database integration
- [`example://http_client/external_api`](../../samples/http_client/) - HTTP client usage

### Alternative Approaches
- [`alternative://sync_to_async_migration`](../memory-bank/main/troubleshooting-guide.md#migration-patterns) - Migrating from synchronous code
- [`alternative://microservice_vs_monolith`](../memory-bank/research/future-directions.md#architecture-patterns) - Service architecture choices

---

**Inheritance**: This rule cannot be overridden and forms the foundation for all other rules.  
**Validation**: All services must comply with these fundamentals.  
**Last Updated**: 2025-01-05  
**Next Review**: 2025-04-05