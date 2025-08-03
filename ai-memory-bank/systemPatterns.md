# System Patterns

## Architecture Overview

The userver framework follows a component-based architecture with coroutine-driven asynchronous processing. The core concepts include:

### Component System
- **Modular Design**: Services are composed of components that can be assembled as needed
- **Lifecycle Management**: Components have well-defined initialization and cleanup phases
- **Dependency Injection**: Components can depend on other components through configuration
- **Configuration-Driven**: Component behavior is controlled through configuration files

### Asynchronous Processing Model
- **Coroutine-Based**: Uses coroutines for non-blocking operations
- **Deadline Propagation**: Request deadlines are automatically propagated through the system
- **Cancellation Support**: Long-running operations can be cancelled when deadlines are exceeded
- **Thread Pool Optimization**: Small number of threads handle many concurrent operations

### HTTP Server Architecture
- **Request Handling Pipeline**: Middleware and handlers process requests in a chain
- **Non-Blocking I/O**: All HTTP operations are asynchronous
- **Connection Management**: Efficient connection pooling for both server and client operations
- **Protocol Support**: HTTP/1.1, HTTP/2, and WebSocket support

## Key Technical Patterns

### 1. Component Pattern
```cpp
class MyHandler : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-my-service";
    using HttpHandlerBase::HttpHandlerBase;
    
    std::string HandleRequest(server::http::HttpRequest& request, 
                              server::request::RequestContext&) const override;
};
```

Components are the building blocks of userver services:
- Each component has a unique name used in configuration
- Components inherit from base classes that provide common functionality
- Components are registered in the component list at startup

### 2. Configuration Pattern
- **Static Configuration**: YAML files defining component configurations
- **Dynamic Configuration**: Runtime-configurable parameters that can be updated without restart
- **Environment Integration**: Configuration values can come from files, environment variables, or remote sources

### 3. Database Integration Patterns
- **Connection Pooling**: Database connections are pooled for efficiency
- **Prepared Statements**: SQL queries are prepared for performance
- **Transaction Management**: Automatic transaction handling with rollback support
- **Async Operations**: All database operations are non-blocking

### 4. Caching Patterns
- **Multiple Cache Types**: In-memory, Redis, and other cache implementations
- **Cache-Aside Pattern**: Load data on cache miss and store for future requests
- **TTL Management**: Automatic cache expiration based on time-to-live settings
- **Cache Warming**: Pre-populate caches with frequently accessed data

### 5. Middleware Pattern
- **Request/Response Interception**: Middleware can modify requests and responses
- **Chaining**: Multiple middleware components can be chained together
- **Order Matters**: Middleware execution order affects behavior
- **Non-blocking**: All middleware must be coroutine-safe

### 6. Error Handling Pattern
- **Structured Exceptions**: Custom exception types for different error categories
- **Graceful Degradation**: Services continue operating when non-critical components fail
- **Circuit Breakers**: Prevent cascading failures in distributed systems
- **Retry Logic**: Automatic retry with exponential backoff for transient failures

## Core Framework Components

### HTTP Server Components
- `server::handlers::HttpHandlerBase`: Base class for HTTP handlers
- `server::http::HttpRequest`: HTTP request object
- `server::http::HttpResponse`: HTTP response object

### Database Components
- `storages::postgres::Cluster`: PostgreSQL cluster management
- `storages::redis::Client`: Redis client implementation
- `storages::mongo::Pool`: MongoDB connection pool

### Utility Components
- `logging::Log`: Structured logging system
- `metrics::Metric`: Metrics collection and reporting
- `tracing::Span`: Distributed tracing implementation
- `cache::CacheContainer`: Cache management

## Design Principles

### 1. Coroutine Safety
All I/O operations must be asynchronous and non-blocking:
```cpp
// Correct - async database operation
auto result = db.Execute("SELECT * FROM users WHERE id=$1", user_id);

// Incorrect - blocking operation that would suspend thread
std::this_thread::sleep_for(1s);
```

### 2. Exception Safety
Use RAII and exception-safe coding practices:
```cpp
// Correct - automatic resource management
auto connection = db.Acquire();
auto result = connection.Execute("SELECT * FROM users");

// Resources automatically released when connection goes out of scope
```

### 3. Configuration-Driven Design
Behavior should be configurable without code changes:
```yaml
# Configuration in YAML
components:
  handler-my-service:
    type: handler-my-service
    task_processor: main-task-processor
    listen_path: /api/v1/users
```

### 4. Observability-First Approach
All components should provide metrics and logging:
```cpp
// Built-in metrics collection
statistics_->GetCounter("requests")->Increment();
statistics_->GetHistogram("request-time")->Record(request_time);
```

## Performance Patterns

### 1. Minimize Context Switches
- Use asynchronous operations to reduce OS context switches
- Optimize memory and CPU usage
- Implement proper deadline handling for requests

### 2. Efficient Resource Usage
- Connection pooling for database and HTTP clients
- Memory pooling for frequently allocated objects
- Lazy initialization of expensive resources

### 3. Deadline Propagation
- HTTP client requests inherit deadlines from incoming requests
- Configure appropriate timeouts based on service level objectives
- Implement circuit breaker patterns for handling flaky external dependencies