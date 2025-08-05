# PostgreSQL Patterns - Database Theme Rules

**Rule ID**: `theme.databases.postgresql_patterns`  
**Priority**: 600  
**Scope**: database-theme  
**Override**: true  
**Inherits**: [`global.framework.fundamentals`](../../00-global/framework-fundamentals.md), [`global.async.programming`](../../00-global/async-programming.md)  
**Specializes**: Database operations for PostgreSQL integration

## Connection Management Patterns

### Connection Pool Configuration
Configure PostgreSQL connection pools for optimal performance:

```yaml
# static_config.yaml - PostgreSQL component configuration
components:
    postgres-db:
        dbalias: postgresql://user:password@localhost:5432/mydb
        blocking_task_processor: fs-task-processor
        dns_resolver: async
        sync-start: true
        
        # Connection pool settings
        min_pool_size: 4
        max_pool_size: 15
        max_queue_size: 200
        
        # Connection timeouts
        connect_timeout: 2s
        statement_timeout: 30s
        
        # Connection lifecycle
        max_prepared_cache_size: 200
        prepared_cache_sample_size: 5
        readonly_master_expected: false
        
        # Cluster configuration for read replicas
        hosts:
          - host: master.db.example.com
            port: 5432
            type: master
          - host: replica1.db.example.com
            port: 5432
            type: slave
          - host: replica2.db.example.com
            port: 5432
            type: slave
```

### Connection Pool Usage Patterns
Use connection pools efficiently in code:

```cpp
namespace myservice::database {

class PostgreSQLManager {
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
public:
    PostgreSQLManager(const userver::components::ComponentContext& context)
        : pg_cluster_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {
    }
    
    // Read operations - use slave connections
    userver::engine::Task<std::vector<User>> GetUsersAsync(size_t limit, size_t offset) const {
        static const auto kQuery = R"(
            SELECT id, email, name, created_at, updated_at 
            FROM users 
            WHERE deleted_at IS NULL 
            ORDER BY created_at DESC 
            LIMIT $1 OFFSET $2
        )";
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,  // Use read replica
            kQuery, limit, offset
        );
        
        std::vector<User> users;
        users.reserve(result.Size());
        
        for (const auto& row : result) {
            users.emplace_back(MapRowToUser(row));
        }
        
        co_return users;
    }
    
    // Write operations - use master connection
    userver::engine::Task<User> CreateUserAsync(const CreateUserRequest& request) const {
        static const auto kQuery = R"(
            INSERT INTO users (id, email, name, created_at, updated_at)
            VALUES ($1, $2, $3, NOW(), NOW())
            RETURNING id, email, name, created_at, updated_at
        )";
        
        auto user_id = userver::utils::generators::GenerateUuid();
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,  // Use master for writes
            kQuery, user_id, request.email, request.name
        );
        
        co_return MapRowToUser(result.AsSingleRow());
    }
};

} // namespace myservice::database
```

**Memory Bank Reference**: [`memory-bank://main/service-patterns#database-integration`](../../memory-bank/main/service-patterns.md#database-integration)

## Transaction Management Patterns

### ACID Transaction Implementation
Implement robust transaction handling:

```cpp
namespace myservice::transactions {

class TransactionManager {
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
public:
    // Simple transaction wrapper
    template<typename Func>
    userver::engine::Task<auto> ExecuteInTransactionAsync(Func&& func) const {
        auto transaction = co_await pg_cluster_->Begin(
            userver::storages::postgres::ClusterHostType::kMaster,
            userver::storages::postgres::TransactionOptions{
                .isolation_level = userver::storages::postgres::IsolationLevel::kReadCommitted,
                .mode = userver::storages::postgres::TransactionMode::kReadWrite
            }
        );
        
        try {
            auto result = co_await func(transaction);
            co_await transaction.Commit();
            co_return result;
            
        } catch (const std::exception& e) {
            try {
                co_await transaction.Rollback();
            } catch (const std::exception& rollback_error) {
                LOG_ERROR() << "Failed to rollback transaction: " << rollback_error.what();
            }
            
            LOG_ERROR() << "Transaction failed and rolled back: " << e.what();
            throw;
        }
    }
    
    // Complex multi-step transaction
    userver::engine::Task<OrderResult> ProcessOrderAsync(const OrderRequest& request) const {
        return ExecuteInTransactionAsync([this, &request](auto& trx) -> userver::engine::Task<OrderResult> {
            // Step 1: Validate inventory
            auto inventory_check = co_await ValidateInventoryAsync(trx, request.items);
            if (!inventory_check.sufficient) {
                throw InsufficientInventoryError(inventory_check.missing_items);
            }
            
            // Step 2: Create order record
            auto order = co_await CreateOrderRecordAsync(trx, request);
            
            // Step 3: Update inventory
            co_await UpdateInventoryAsync(trx, request.items);
            
            // Step 4: Create order items
            for (const auto& item : request.items) {
                co_await CreateOrderItemAsync(trx, order.id, item);
            }
            
            // Step 5: Calculate totals
            auto total = co_await CalculateOrderTotalAsync(trx, order.id);
            co_await UpdateOrderTotalAsync(trx, order.id, total);
            
            co_return OrderResult{
                .order_id = order.id,
                .total_amount = total,
                .status = OrderStatus::kCreated
            };
        });
    }

private:
    userver::engine::Task<InventoryCheck> ValidateInventoryAsync(
        userver::storages::postgres::Transaction& trx,
        const std::vector<OrderItem>& items) const {
        
        static const auto kQuery = R"(
            SELECT product_id, available_quantity 
            FROM inventory 
            WHERE product_id = ANY($1)
            FOR UPDATE  -- Lock rows for update
        )";
        
        std::vector<std::string> product_ids;
        for (const auto& item : items) {
            product_ids.push_back(item.product_id);
        }
        
        auto result = co_await trx.Execute(kQuery, product_ids);
        
        InventoryCheck check{.sufficient = true};
        std::unordered_map<std::string, int> available_inventory;
        
        for (const auto& row : result) {
            available_inventory[row["product_id"].As<std::string>()] = 
                row["available_quantity"].As<int>();
        }
        
        for (const auto& item : items) {
            auto available = available_inventory[item.product_id];
            if (available < item.quantity) {
                check.sufficient = false;
                check.missing_items.push_back({
                    .product_id = item.product_id,
                    .requested = item.quantity,
                    .available = available
                });
            }
        }
        
        co_return check;
    }
};

} // namespace myservice::transactions
```

### Distributed Transaction Patterns
Handle distributed transactions across multiple resources:

```cpp
namespace myservice::distributed {

class DistributedTransactionManager {
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    userver::storages::redis::ClientPtr redis_client_;
    ExternalPaymentService& payment_service_;
    
public:
    // Two-phase commit pattern for distributed transactions
    userver::engine::Task<PaymentResult> ProcessPaymentAsync(
        const PaymentRequest& request) const {
        
        auto transaction_id = userver::utils::generators::GenerateUuid();
        
        // Phase 1: Prepare all resources
        auto pg_transaction = co_await pg_cluster_->Begin(
            userver::storages::postgres::ClusterHostType::kMaster,
            userver::storages::postgres::TransactionOptions{}
        );
        
        try {
            // Prepare database changes
            co_await PreparePaymentRecordAsync(pg_transaction, request, transaction_id);
            
            // Prepare Redis state
            co_await redis_client_->SetAsync(
                fmt::format("payment_prepare:{}", transaction_id),
                SerializePaymentState(request),
                std::chrono::minutes(5)  // Timeout for preparation
            );
            
            // Prepare external payment service
            auto payment_preparation = co_await payment_service_.PreparePaymentAsync(
                request, transaction_id
            );
            
            if (!payment_preparation.success) {
                throw PaymentPreparationError(payment_preparation.error_message);
            }
            
            // Phase 2: Commit all resources
            co_await CommitDistributedTransactionAsync(
                pg_transaction, transaction_id, payment_preparation.payment_token
            );
            
            co_return PaymentResult{
                .success = true,
                .transaction_id = transaction_id,
                .payment_token = payment_preparation.payment_token
            };
            
        } catch (const std::exception& e) {
            // Rollback all resources
            co_await RollbackDistributedTransactionAsync(
                pg_transaction, transaction_id
            );
            throw;
        }
    }

private:
    userver::engine::Task<void> CommitDistributedTransactionAsync(
        userver::storages::postgres::Transaction& pg_transaction,
        const std::string& transaction_id,
        const std::string& payment_token) const {
        
        try {
            // Commit database transaction
            co_await pg_transaction.Commit();
            
            // Commit external payment
            co_await payment_service_.CommitPaymentAsync(payment_token);
            
            // Clean up Redis preparation state
            co_await redis_client_->DelAsync(fmt::format("payment_prepare:{}", transaction_id));
            
            // Record successful transaction
            co_await redis_client_->SetAsync(
                fmt::format("payment_success:{}", transaction_id),
                "committed",
                std::chrono::hours(24)
            );
            
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to commit distributed transaction: " << e.what()
                        << userver::logging::LogExtra{{"transaction_id", transaction_id}};
            
            // Attempt compensation
            co_await CompensateFailedCommitAsync(transaction_id, payment_token);
            throw;
        }
    }
};

} // namespace myservice::distributed
```

**Cross-Reference**: [`pattern://transactions/distributed_transactions`](../../memory-bank/research/new-patterns.md#distributed-patterns)

## Query Optimization Patterns

### Prepared Statement Management
Use prepared statements for performance:

```cpp
namespace myservice::queries {

class OptimizedQueryManager {
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
    // Prepared statement cache
    static inline const std::unordered_map<std::string, std::string> kPreparedQueries = {
        {"find_user_by_id", R"(
            SELECT id, email, name, created_at, updated_at 
            FROM users 
            WHERE id = $1 AND deleted_at IS NULL
        )"},
        
        {"find_users_by_email_domain", R"(
            SELECT id, email, name, created_at, updated_at 
            FROM users 
            WHERE email LIKE '%' || $1 AND deleted_at IS NULL 
            ORDER BY created_at DESC 
            LIMIT $2 OFFSET $3
        )"},
        
        {"update_user_last_login", R"(
            UPDATE users 
            SET last_login_at = NOW(), updated_at = NOW() 
            WHERE id = $1 AND deleted_at IS NULL
            RETURNING last_login_at
        )"},
        
        {"get_user_statistics", R"(
            SELECT 
                COUNT(*) as total_users,
                COUNT(*) FILTER (WHERE created_at >= NOW() - INTERVAL '30 days') as new_users_30d,
                COUNT(*) FILTER (WHERE last_login_at >= NOW() - INTERVAL '7 days') as active_users_7d
            FROM users 
            WHERE deleted_at IS NULL
        )"}
    };
    
public:
    // Optimized single user lookup
    userver::engine::Task<std::optional<User>> FindUserByIdAsync(
        const std::string& user_id) const {
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kPreparedQueries.at("find_user_by_id"),
            user_id
        );
        
        if (result.IsEmpty()) {
            co_return std::nullopt;
        }
        
        co_return MapRowToUser(result.AsSingleRow());
    }
    
    // Batch operations for better performance
    userver::engine::Task<std::vector<User>> FindUsersByIdsAsync(
        const std::vector<std::string>& user_ids) const {
        
        if (user_ids.empty()) {
            co_return std::vector<User>{};
        }
        
        // Use ANY operator for efficient batch lookup
        static const auto kBatchQuery = R"(
            SELECT id, email, name, created_at, updated_at 
            FROM users 
            WHERE id = ANY($1) AND deleted_at IS NULL
            ORDER BY created_at DESC
        )";
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kBatchQuery,
            user_ids
        );
        
        std::vector<User> users;
        users.reserve(result.Size());
        
        for (const auto& row : result) {
            users.emplace_back(MapRowToUser(row));
        }
        
        co_return users;
    }
    
    // Complex analytical query with proper indexing
    userver::engine::Task<UserStatistics> GetUserStatisticsAsync() const {
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kPreparedQueries.at("get_user_statistics")
        );
        
        const auto& row = result.AsSingleRow();
        
        co_return UserStatistics{
            .total_users = row["total_users"].As<int64_t>(),
            .new_users_30d = row["new_users_30d"].As<int64_t>(),
            .active_users_7d = row["active_users_7d"].As<int64_t>()
        };
    }
};

} // namespace myservice::queries
```

### Index Strategy Implementation
Implement database indexing strategies:

```sql
-- Database migration: Create optimized indexes
-- File: migrations/001_create_users_indexes.sql

-- Primary key index (automatically created)
-- CREATE UNIQUE INDEX users_pkey ON users (id);

-- Email lookup index (unique constraint)
CREATE UNIQUE INDEX CONCURRENTLY idx_users_email 
ON users (email) 
WHERE deleted_at IS NULL;

-- Composite index for common query patterns
CREATE INDEX CONCURRENTLY idx_users_created_at_active 
ON users (created_at DESC, deleted_at) 
WHERE deleted_at IS NULL;

-- Partial index for active users
CREATE INDEX CONCURRENTLY idx_users_last_login_active 
ON users (last_login_at DESC) 
WHERE deleted_at IS NULL AND last_login_at IS NOT NULL;

-- Full-text search index for name and email
CREATE INDEX CONCURRENTLY idx_users_search 
ON users USING gin(to_tsvector('english', name || ' ' || email)) 
WHERE deleted_at IS NULL;

-- Analyze tables after index creation
ANALYZE users;
```

```cpp
// Query patterns that leverage the indexes
namespace myservice::indexed_queries {

class IndexOptimizedQueries {
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
public:
    // Leverages idx_users_email index
    userver::engine::Task<std::optional<User>> FindUserByEmailAsync(
        const std::string& email) const {
        
        static const auto kQuery = R"(
            SELECT id, email, name, created_at, updated_at 
            FROM users 
            WHERE email = $1 AND deleted_at IS NULL
        )";
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kQuery, email
        );
        
        if (result.IsEmpty()) {
            co_return std::nullopt;
        }
        
        co_return MapRowToUser(result.AsSingleRow());
    }
    
    // Leverages idx_users_created_at_active index
    userver::engine::Task<std::vector<User>> GetRecentUsersAsync(
        size_t limit, size_t offset) const {
        
        static const auto kQuery = R"(
            SELECT id, email, name, created_at, updated_at 
            FROM users 
            WHERE deleted_at IS NULL 
            ORDER BY created_at DESC 
            LIMIT $1 OFFSET $2
        )";
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kQuery, limit, offset
        );
        
        std::vector<User> users;
        users.reserve(result.Size());
        
        for (const auto& row : result) {
            users.emplace_back(MapRowToUser(row));
        }
        
        co_return users;
    }
    
    // Leverages idx_users_search full-text index
    userver::engine::Task<std::vector<User>> SearchUsersAsync(
        const std::string& search_term, size_t limit) const {
        
        static const auto kQuery = R"(
            SELECT id, email, name, created_at, updated_at,
                   ts_rank(to_tsvector('english', name || ' ' || email), 
                          plainto_tsquery('english', $1)) as rank
            FROM users 
            WHERE deleted_at IS NULL 
              AND to_tsvector('english', name || ' ' || email) @@ plainto_tsquery('english', $1)
            ORDER BY rank DESC, created_at DESC
            LIMIT $2
        )";
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kQuery, search_term, limit
        );
        
        std::vector<User> users;
        users.reserve(result.Size());
        
        for (const auto& row : result) {
            users.emplace_back(MapRowToUser(row));
        }
        
        co_return users;
    }
};

} // namespace myservice::indexed_queries
```

**Memory Bank Reference**: [`memory-bank://research/performance-research#database-optimization`](../../memory-bank/research/performance-research.md#database-optimization)

## Error Handling and Resilience

### Database Error Recovery Patterns
Implement robust error handling for database operations:

```cpp
namespace myservice::resilience {

class DatabaseResilienceManager {
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    userver::storages::redis::ClientPtr redis_client_;
    
    struct RetryConfig {
        size_t max_attempts = 3;
        std::chrono::milliseconds initial_delay{100};
        double backoff_multiplier = 2.0;
        std::chrono::milliseconds max_delay{5000};
    };
    
    RetryConfig retry_config_;
    
public:
    // Retry wrapper for transient database errors
    template<typename Func>
    userver::engine::Task<auto> ExecuteWithRetryAsync(Func&& func) const {
        size_t attempt = 0;
        auto delay = retry_config_.initial_delay;
        
        while (true) {
            try {
                co_return co_await func();
                
            } catch (const userver::storages::postgres::ConnectionError& e) {
                ++attempt;
                if (attempt >= retry_config_.max_attempts) {
                    LOG_ERROR() << "Database connection failed after " << attempt << " attempts: " << e.what();
                    throw DatabaseUnavailableError("Database connection failed after retries");
                }
                
                LOG_WARNING() << "Database connection error, retrying in " << delay.count() << "ms (attempt " 
                             << attempt << "/" << retry_config_.max_attempts << "): " << e.what();
                
                co_await userver::engine::SleepFor(delay);
                delay = std::min(
                    static_cast<std::chrono::milliseconds>(delay.count() * retry_config_.backoff_multiplier),
                    retry_config_.max_delay
                );
                
            } catch (const userver::storages::postgres::TransactionRollbackError& e) {
                ++attempt;
                if (attempt >= retry_config_.max_attempts) {
                    LOG_ERROR() << "Transaction rollback after " << attempt << " attempts: " << e.what();
                    throw;
                }
                
                LOG_WARNING() << "Transaction rollback, retrying (attempt " 
                             << attempt << "/" << retry_config_.max_attempts << "): " << e.what();
                
                co_await userver::engine::SleepFor(delay);
                delay = std::min(
                    static_cast<std::chrono::milliseconds>(delay.count() * retry_config_.backoff_multiplier),
                    retry_config_.max_delay
                );
                
            } catch (const userver::storages::postgres::Error& e) {
                // Non-retryable database errors
                LOG_ERROR() << "Non-retryable database error: " << e.what();
                throw DatabaseError(fmt::format("Database operation failed: {}", e.what()));
            }
        }
    }
    
    // Circuit breaker pattern for database operations
    userver::engine::Task<std::optional<User>> GetUserWithFallbackAsync(
        const std::string& user_id) const {
        
        try {
            // Try primary database
            return co_await ExecuteWithRetryAsync([this, &user_id]() {
                return GetUserFromDatabaseAsync(user_id);
            });
            
        } catch (const DatabaseUnavailableError& e) {
            LOG_WARNING() << "Primary database unavailable, trying cache fallback: " << e.what();
            
            // Fallback to cache
            auto cached_user = co_await GetUserFromCacheAsync(user_id);
            if (cached_user) {
                LOG_INFO() << "Retrieved user from cache fallback"
                           << userver::logging::LogExtra{{"user_id", user_id}};
                co_return cached_user;
            }
            
            LOG_ERROR() << "User not found in cache fallback"
                        << userver::logging::LogExtra{{"user_id", user_id}};
            co_return std::nullopt;
        }
    }

private:
    userver::engine::Task<std::optional<User>> GetUserFromDatabaseAsync(
        const std::string& user_id) const {
        
        static const auto kQuery = R"(
            SELECT id, email, name, created_at, updated_at 
            FROM users 
            WHERE id = $1 AND deleted_at IS NULL
        )";
        
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kQuery, user_id
        );
        
        if (result.IsEmpty()) {
            co_return std::nullopt;
        }
        
        co_return MapRowToUser(result.AsSingleRow());
    }
    
    userver::engine::Task<std::optional<User>> GetUserFromCacheAsync(
        const std::string& user_id) const {
        
        auto cache_key = fmt::format("user:{}", user_id);
        auto cached_data = co_await redis_client_->GetAsync(cache_key);
        
        if (!cached_data) {
            co_return std::nullopt;
        }
        
        co_return DeserializeUser(*cached_data);
    }
};

} // namespace myservice::resilience
```

**Cross-Reference**: [`pattern://resilience/database_fallback`](../../memory-bank/specialized/chaos-testing/chaos-patterns.md#database-resilience)

## Performance Monitoring and Metrics

### Database Performance Metrics
Implement comprehensive database monitoring:

```cpp
namespace myservice::monitoring {

class DatabaseMetricsCollector {
private:
    userver::utils::statistics::MetricStorage& metrics_storage_;
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
public:
    DatabaseMetricsCollector(
        userver::utils::statistics::MetricStorage& metrics_storage,
        userver::storages::postgres::ClusterPtr pg_cluster)
        : metrics_storage_(metrics_storage), pg_cluster_(std::move(pg_cluster)) {
    }
    
    // Wrap database operations with metrics collection
    template<typename Func>
    userver::engine::Task<auto> MeasureQueryAsync(
        const std::string& query_name, Func&& func) const {
        
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            auto result = co_await func();
            
            // Record successful query metrics
            auto duration = std::chrono::steady_clock::now() - start_time;
            RecordQueryMetrics(query_name, duration, true);
            
            co_return result;
            
        } catch (const std::exception& e) {
            // Record failed query metrics
            auto duration = std::chrono::steady_clock::now() - start_time;
            RecordQueryMetrics(query_name, duration, false);
            
            throw;
        }
    }
    
    // Collect database connection pool metrics
    userver::engine::Task<void> CollectConnectionPoolMetricsAsync() const {
        auto stats = pg_cluster_->GetStatistics();
        
        metrics_storage_.GetMetric("postgres.connections.active").Set(stats.active_connections);
        metrics_storage_.GetMetric("postgres.connections.used").Set(stats.used_connections);
        metrics_storage_.GetMetric("postgres.connections.maximum").Set(stats.max_connections);
        metrics_storage_.GetMetric("postgres.connections.waiting").Set(stats.waiting_connections);
        
        metrics_storage_.GetMetric("postgres.queries.total").Set(stats.total_queries);
        metrics_storage_.GetMetric("postgres.queries.failed").Set(stats.failed_queries);
        
        LOG_DEBUG() << "Database connection pool metrics collected"
                    << userver::logging::LogExtra{
                        {"active_connections", stats.active_connections},
                        {"used_connections", stats.used_connections},
                        {"waiting_connections", stats.waiting_connections}
                    };
    }

private:
    void RecordQueryMetrics(const std::string& query_name, 
                           std::chrono::steady_clock::duration duration,
                           bool success) const {
        
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        // Query duration histogram
        metrics_storage_.GetMetric(fmt::format("postgres.query.duration.{}", query_name))
            .Record(duration_ms);
        
        // Query count by status
        if (success) {
            metrics_storage_.GetMetric(fmt::format("postgres.query.success.{}", query_name)).Inc();
        } else {
            metrics_storage_.GetMetric(fmt::format("postgres.query.error.{}", query_name)).Inc();
        }
        
        // Overall query metrics
        metrics_storage_.GetMetric("postgres.query.total").Inc();
        if (!success) {
            metrics_storage_.GetMetric("postgres.query.errors").Inc();
        }
    }
};

// Usage example with metrics
class MetricsAwareUserRepository {
private:
    DatabaseMetricsCollector metrics_collector_;
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
public:
    userver::engine::Task<std::optional<User>> FindUserByIdAsync(
        const std::string& user_id) const {
        
        return metrics_collector_.MeasureQueryAsync("find_user_by_id", [this, &user_id]() {
            return ExecuteFindUserQuery(user_id);
        });
    }
    
    userver::engine::Task<User> CreateUserAsync(const CreateUserRequest& request) const {
        return metrics_collector_.MeasureQueryAsync("create_user", [this, &request]() {
            return ExecuteCreateUserQuery(request);
        });
    }
};

} // namespace myservice::monitoring
```

**Memory Bank Reference**: [`memory-bank://specialized/advanced-monitoring#database-monitoring`](../../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md#database-monitoring)

## Cross-References

### Related Memory Bank Entries
- [`memory-bank://main/service-patterns#database-integration`](../../memory-bank/main/service-patterns.md#database-integration) - Database integration patterns
- [`memory-bank://research/performance-research#database-optimization`](../../memory-bank/research/performance-research.md#database-optimization) - Performance optimization
- [`memory-bank://specialized/chaos-testing#database-resilience`](../../memory-bank/specialized/chaos-testing/chaos-patterns.md#database-resilience) - Resilience testing

### Implementation Examples
- [`example://database/postgresql_integration`](../../../samples/postgres_service/) - Complete PostgreSQL integration
- [`example://database/transaction_management`](../../../samples/production_service/) - Transaction patterns
- [`example://database/performance_optimization`](../../../samples/cache_service/) - Performance patterns

### Alternative Approaches
- [`alternative://orm_vs_raw_sql`](../../memory-bank/research/new-patterns.md#data-access-patterns) - Data access strategies
- [`alternative://read_replica_strategies`](../../memory-bank/research/performance-research.md#scaling-patterns) - Read scaling approaches

---

**Theme Context**: Database theme provides specialized PostgreSQL integration expertise.  
**Inheritance**: Extends global rules with database-specific patterns.  
**Dependencies**: [`global.framework.fundamentals`](../../00-global/framework-fundamentals.md), [`global.async.programming`](../../00-global/async-programming.md)  
**Last Updated**: 2025-01-05  
**Next Review**: 2025-04-05