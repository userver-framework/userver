# System Design - Architect Mode Rules

**Rule ID**: `mode.architect.system_design`  
**Priority**: 800  
**Scope**: architect-mode  
**Override**: true  
**Inherits**: [`global.framework.fundamentals`](../../00-global/framework-fundamentals.md), [`global.component.system`](../../00-global/component-system.md)

## Architecture Planning Methodology

### Service Architecture Patterns
Design userver services using proven architectural patterns:

#### Microservice Architecture
```yaml
# Service decomposition strategy
services:
  user-service:
    responsibilities:
      - User authentication and authorization
      - User profile management
      - User preferences
    dependencies:
      - postgres-user-db
      - redis-session-cache
    
  order-service:
    responsibilities:
      - Order processing
      - Payment integration
      - Order history
    dependencies:
      - postgres-order-db
      - kafka-events
      - user-service (via HTTP)
```

#### Modular Monolith Pattern
```cpp
// Organize large services into modules
namespace user_management {
    class UserComponent;
    class AuthenticationHandler;
    class ProfileHandler;
}

namespace order_processing {
    class OrderComponent;
    class PaymentHandler;
    class OrderHistoryHandler;
}

// Clear module boundaries with defined interfaces
class UserServiceInterface {
public:
    virtual ~UserServiceInterface() = default;
    virtual userver::engine::Task<User> GetUserAsync(UserId id) = 0;
    virtual userver::engine::Task<bool> ValidateTokenAsync(const Token& token) = 0;
};
```

**Memory Bank Reference**: [`memory-bank://main/service-patterns#architecture-patterns`](../../memory-bank/main/service-patterns.md#architecture-patterns)

### Component Interaction Design

#### Service Communication Patterns
Design communication between services:

```cpp
// HTTP-based service communication
class ExternalServiceClient {
private:
    userver::clients::http::Client& http_client_;
    std::string base_url_;
    
public:
    userver::engine::Task<UserProfile> GetUserProfileAsync(UserId user_id) {
        auto request = http_client_.CreateRequest()
            .get(fmt::format("{}/users/{}", base_url_, user_id))
            .timeout(std::chrono::seconds(5))
            .retry(3);
            
        auto response = co_await request.perform();
        co_return ParseUserProfile(response.body());
    }
};

// Event-driven communication via messaging
class EventPublisher {
private:
    userver::kafka::Producer& kafka_producer_;
    
public:
    userver::engine::Task<void> PublishUserCreatedEvent(const User& user) {
        UserCreatedEvent event{
            .user_id = user.id,
            .email = user.email,
            .created_at = std::chrono::system_clock::now()
        };
        
        co_await kafka_producer_.SendAsync("user-events", SerializeEvent(event));
    }
};
```

#### Database Integration Strategy
Plan database interactions and transactions:

```cpp
// Multi-database transaction coordination
class OrderService {
private:
    userver::storages::postgres::Cluster& pg_cluster_;
    userver::storages::redis::Client& redis_client_;
    
public:
    userver::engine::Task<OrderResult> CreateOrderAsync(const OrderRequest& request) {
        // Start distributed transaction
        auto trx = co_await pg_cluster_.Begin(
            userver::storages::postgres::ClusterHostType::kMaster,
            userver::storages::postgres::TransactionOptions{}
        );
        
        try {
            // Create order record
            auto order = co_await CreateOrderRecord(trx, request);
            
            // Update inventory
            co_await UpdateInventory(trx, request.items);
            
            // Cache order for quick access
            co_await redis_client_.SetAsync(
                fmt::format("order:{}", order.id),
                SerializeOrder(order),
                std::chrono::minutes(30)
            );
            
            co_await trx.Commit();
            co_return OrderResult{.success = true, .order = order};
            
        } catch (const std::exception& e) {
            co_await trx.Rollback();
            throw;
        }
    }
};
```

**Cross-Reference**: [`pattern://database/transaction_management`](../../memory-bank/main/service-patterns.md#database-patterns)

## Scalability Architecture

### Horizontal Scaling Design
Design services for horizontal scalability:

```yaml
# Load balancing configuration
load_balancer:
  algorithm: round_robin
  health_check:
    path: /ping
    interval: 30s
    timeout: 5s
  
  backends:
    - host: service-instance-1:8080
      weight: 1
    - host: service-instance-2:8080
      weight: 1
    - host: service-instance-3:8080
      weight: 1

# Auto-scaling configuration
auto_scaling:
  min_instances: 2
  max_instances: 10
  target_cpu_utilization: 70%
  scale_up_cooldown: 300s
  scale_down_cooldown: 600s
```

### Caching Strategy Design
Plan multi-level caching architecture:

```cpp
// Multi-level caching strategy
class CachingStrategy {
private:
    userver::cache::LruCache<std::string, UserProfile> l1_cache_;  // In-memory
    userver::storages::redis::Client& l2_cache_;                   // Redis
    UserDatabase& database_;                                       // Source of truth
    
public:
    userver::engine::Task<UserProfile> GetUserProfileAsync(UserId user_id) {
        auto key = fmt::format("user_profile:{}", user_id);
        
        // L1 Cache (in-memory)
        if (auto cached = l1_cache_.Get(key)) {
            co_return *cached;
        }
        
        // L2 Cache (Redis)
        auto redis_value = co_await l2_cache_.GetAsync(key);
        if (redis_value) {
            auto profile = DeserializeUserProfile(*redis_value);
            l1_cache_.Put(key, profile);
            co_return profile;
        }
        
        // Database (source of truth)
        auto profile = co_await database_.GetUserProfileAsync(user_id);
        
        // Populate caches
        co_await l2_cache_.SetAsync(key, SerializeUserProfile(profile), 
                                   std::chrono::hours(1));
        l1_cache_.Put(key, profile);
        
        co_return profile;
    }
};
```

**Memory Bank Reference**: [`memory-bank://specialized/advanced-monitoring#caching-strategies`](../../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md#caching-strategies)

## Performance Architecture

### Latency Optimization Design
Design for low-latency operations:

```cpp
// Connection pooling for database performance
class OptimizedDatabaseAccess {
private:
    struct ConnectionPoolConfig {
        size_t min_connections = 5;
        size_t max_connections = 20;
        std::chrono::seconds connection_timeout{30};
        std::chrono::seconds idle_timeout{300};
    };
    
    userver::storages::postgres::Cluster& pg_cluster_;
    ConnectionPoolConfig config_;
    
public:
    // Prepared statements for frequently used queries
    userver::engine::Task<User> GetUserByIdAsync(UserId user_id) {
        static const auto kQuery = "SELECT id, email, name, created_at FROM users WHERE id = $1";
        
        auto result = co_await pg_cluster_.Execute(
            userver::storages::postgres::ClusterHostType::kSlave,  // Read from replica
            kQuery, user_id
        );
        
        co_return ParseUser(result.AsSingleRow<User>());
    }
};

// Async batch processing for throughput
class BatchProcessor {
private:
    static constexpr size_t kBatchSize = 100;
    static constexpr auto kBatchTimeout = std::chrono::milliseconds{50};
    
public:
    userver::engine::Task<std::vector<Result>> ProcessBatchAsync(
        const std::vector<Item>& items) {
        
        std::vector<userver::engine::Task<Result>> tasks;
        tasks.reserve(items.size());
        
        // Process in parallel batches
        for (size_t i = 0; i < items.size(); i += kBatchSize) {
            auto batch_end = std::min(i + kBatchSize, items.size());
            
            tasks.emplace_back(ProcessBatch(
                std::vector<Item>(items.begin() + i, items.begin() + batch_end)
            ));
        }
        
        // Collect results
        std::vector<Result> results;
        for (auto& task : tasks) {
            auto batch_results = co_await task;
            results.insert(results.end(), batch_results.begin(), batch_results.end());
        }
        
        co_return results;
    }
};
```

### Resource Management Architecture
Design efficient resource utilization:

```yaml
# Resource allocation strategy
resource_allocation:
  task_processors:
    main-task-processor:
      worker_threads: 8           # CPU cores * 2
      thread_name: main-worker
      
    io-task-processor:
      worker_threads: 4           # I/O bound operations
      thread_name: io-worker
      
    background-task-processor:
      worker_threads: 2           # Background tasks
      thread_name: bg-worker
      
  memory_management:
    heap_size: 2GB
    gc_strategy: incremental
    
  connection_pools:
    postgres:
      min_connections: 5
      max_connections: 20
      
    redis:
      min_connections: 2
      max_connections: 10
```

**Cross-Reference**: [`concept://performance/resource_optimization`](../../memory-bank/research/performance-research.md#resource-optimization)

## Security Architecture

### Authentication and Authorization Design
Design secure authentication flows:

```cpp
// JWT-based authentication architecture
class AuthenticationService {
private:
    std::string jwt_secret_;
    std::chrono::seconds token_expiry_;
    userver::storages::redis::Client& session_store_;
    
public:
    struct AuthenticationResult {
        bool success;
        std::string access_token;
        std::string refresh_token;
        User user;
    };
    
    userver::engine::Task<AuthenticationResult> AuthenticateAsync(
        const std::string& email, const std::string& password) {
        
        // Validate credentials
        auto user = co_await ValidateCredentialsAsync(email, password);
        if (!user) {
            co_return AuthenticationResult{.success = false};
        }
        
        // Generate tokens
        auto access_token = GenerateAccessToken(*user);
        auto refresh_token = GenerateRefreshToken(*user);
        
        // Store session
        co_await session_store_.SetAsync(
            fmt::format("session:{}", user->id),
            SerializeSession({.user_id = user->id, .created_at = std::chrono::system_clock::now()}),
            token_expiry_
        );
        
        co_return AuthenticationResult{
            .success = true,
            .access_token = access_token,
            .refresh_token = refresh_token,
            .user = *user
        };
    }
};

// Role-based access control
class AuthorizationService {
private:
    enum class Permission {
        READ_USER,
        WRITE_USER,
        DELETE_USER,
        ADMIN_ACCESS
    };
    
    std::unordered_map<Role, std::set<Permission>> role_permissions_;
    
public:
    bool HasPermission(const User& user, Permission permission) const {
        auto it = role_permissions_.find(user.role);
        return it != role_permissions_.end() && 
               it->second.contains(permission);
    }
};
```

### Data Protection Architecture
Design data encryption and protection:

```cpp
// Data encryption at rest and in transit
class DataProtectionService {
private:
    userver::crypto::Cipher cipher_;
    std::string encryption_key_;
    
public:
    // Encrypt sensitive data before storage
    std::string EncryptSensitiveData(const std::string& data) {
        return cipher_.Encrypt(data, encryption_key_);
    }
    
    // Decrypt data after retrieval
    std::string DecryptSensitiveData(const std::string& encrypted_data) {
        return cipher_.Decrypt(encrypted_data, encryption_key_);
    }
    
    // Hash passwords securely
    std::string HashPassword(const std::string& password, const std::string& salt) {
        return userver::crypto::hash::Pbkdf2HmacSha256(password, salt, 10000);
    }
};
```

**Memory Bank Reference**: [`memory-bank://specialized/security-patterns`](../../memory-bank/research/new-patterns.md#security-patterns)

## Integration Architecture

### External Service Integration
Design resilient external service integration:

```cpp
// Circuit breaker pattern for external services
class ExternalServiceClient {
private:
    userver::clients::http::Client& http_client_;
    CircuitBreaker circuit_breaker_;
    
    struct CircuitBreakerConfig {
        size_t failure_threshold = 5;
        std::chrono::seconds timeout{30};
        std::chrono::seconds recovery_timeout{60};
    };
    
public:
    userver::engine::Task<Response> CallExternalServiceAsync(const Request& request) {
        if (circuit_breaker_.IsOpen()) {
            throw ServiceUnavailableError("External service circuit breaker is open");
        }
        
        try {
            auto response = co_await http_client_.CreateRequest()
                .post(external_url_, SerializeRequest(request))
                .timeout(std::chrono::seconds(10))
                .retry(3)
                .perform();
                
            circuit_breaker_.RecordSuccess();
            co_return ParseResponse(response.body());
            
        } catch (const std::exception& e) {
            circuit_breaker_.RecordFailure();
            throw;
        }
    }
};

// Event-driven integration via message queues
class EventDrivenIntegration {
private:
    userver::kafka::Consumer& kafka_consumer_;
    userver::kafka::Producer& kafka_producer_;
    
public:
    // Process incoming events
    userver::engine::Task<void> ProcessEventsAsync() {
        while (true) {
            auto messages = co_await kafka_consumer_.ReceiveAsync();
            
            for (const auto& message : messages) {
                try {
                    co_await ProcessEventMessage(message);
                } catch (const std::exception& e) {
                    LOG_ERROR() << "Failed to process event: " << e.what();
                    // Send to dead letter queue
                    co_await kafka_producer_.SendAsync("dead-letter-queue", message);
                }
            }
        }
    }
};
```

## Monitoring and Observability Architecture

### Metrics and Monitoring Design
Design comprehensive monitoring strategy:

```cpp
// Metrics collection architecture
class MetricsCollector {
private:
    userver::utils::statistics::MetricStorage& metrics_storage_;
    
public:
    void RecordRequestMetrics(const std::string& endpoint, 
                             std::chrono::milliseconds duration,
                             int status_code) {
        // Request count
        metrics_storage_.GetMetric(fmt::format("requests_total.{}", endpoint)).Inc();
        
        // Request duration
        metrics_storage_.GetMetric(fmt::format("request_duration.{}", endpoint))
            .Record(duration.count());
        
        // Status code distribution
        metrics_storage_.GetMetric(fmt::format("status_codes.{}.{}", endpoint, status_code))
            .Inc();
    }
    
    void RecordBusinessMetrics(const std::string& metric_name, double value) {
        metrics_storage_.GetMetric(fmt::format("business.{}", metric_name))
            .Record(value);
    }
};

// Distributed tracing architecture
class TracingService {
private:
    userver::tracing::Tracer& tracer_;
    
public:
    userver::engine::Task<Result> TracedOperation(const std::string& operation_name) {
        auto span = tracer_.StartSpan(operation_name);
        
        try {
            auto result = co_await PerformOperation();
            span.SetTag("result.success", true);
            co_return result;
        } catch (const std::exception& e) {
            span.SetTag("result.success", false);
            span.SetTag("error.message", e.what());
            throw;
        }
    }
};
```

**Memory Bank Reference**: [`memory-bank://specialized/advanced-monitoring`](../../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md)

## Cross-References

### Related Memory Bank Entries
- [`memory-bank://main/service-patterns`](../../memory-bank/main/service-patterns.md) - Service design patterns
- [`memory-bank://main/component-system`](../../memory-bank/main/component-system.md) - Component architecture
- [`memory-bank://research/future-directions`](../../memory-bank/research/future-directions.md) - Architecture evolution

### Implementation Examples
- [`example://architecture/microservice`](../../../samples/production_service/) - Microservice implementation
- [`example://architecture/caching`](../../../samples/cache_service/) - Caching architecture
- [`example://architecture/monitoring`](../../../samples/monitoring_service/) - Monitoring integration

### Alternative Approaches
- [`alternative://monolith_vs_microservice`](../../memory-bank/research/future-directions.md#architecture-choices) - Architecture trade-offs
- [`alternative://sync_vs_async_architecture`](../../memory-bank/main/async-programming.md#architecture-patterns) - Concurrency models

---

**Mode Context**: Architect mode focuses on high-level design and system architecture.  
**Inheritance**: Extends global rules with architecture-specific guidance.  
**Dependencies**: [`global.framework.fundamentals`](../../00-global/framework-fundamentals.md), [`global.component.system`](../../00-global/component-system.md)  
**Last Updated**: 2025-01-05  
**Next Review**: 2025-04-05