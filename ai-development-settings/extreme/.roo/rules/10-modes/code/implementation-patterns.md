# Implementation Patterns - Code Mode Rules

**Rule ID**: `mode.code.implementation_patterns`  
**Priority**: 800  
**Scope**: code-mode  
**Override**: true  
**Inherits**: [`global.framework.fundamentals`](../../00-global/framework-fundamentals.md), [`global.component.system`](../../00-global/component-system.md), [`global.async.programming`](../../00-global/async-programming.md)

## HTTP Handler Implementation

### Standard Handler Pattern
Implement HTTP handlers following userver conventions:

```cpp
#pragma once

#include <userver/server/handlers/http_handler_base.hpp>

namespace myservice::handlers {

class UserProfileHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-profile";

    UserProfileHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    userver::storages::redis::ClientPtr redis_client_;
    
    userver::engine::Task<UserProfile> GetUserProfileAsync(
        const std::string& user_id) const;
    
    userver::engine::Task<void> CacheUserProfileAsync(
        const std::string& user_id, const UserProfile& profile) const;
};

} // namespace myservice::handlers
```

```cpp
// Implementation file
#include "user_profile_handler.hpp"

#include <fmt/format.h>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/logging/log.hpp>

namespace myservice::handlers {

UserProfileHandler::UserProfileHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_client_(context.FindComponent<userver::components::Redis>("redis").GetClient()) {
}

std::string UserProfileHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    
    // Extract and validate parameters
    const auto user_id = request.GetPathArg("user_id");
    if (user_id.empty()) {
        throw userver::server::handlers::ClientError(
            userver::server::handlers::ExternalBody{"Missing user_id parameter"}
        );
    }
    
    // Log request with correlation ID
    LOG_INFO() << "Processing user profile request"
               << userver::logging::LogExtra{
                   {"user_id", user_id},
                   {"method", request.GetMethod()},
                   {"remote_addr", request.GetRemoteAddress()}
               };
    
    try {
        switch (request.GetMethod()) {
            case userver::server::http::HttpMethod::kGet:
                return HandleGetProfile(user_id).Get();
            case userver::server::http::HttpMethod::kPut:
                return HandleUpdateProfile(request, user_id).Get();
            default:
                throw userver::server::handlers::ClientError(
                    userver::server::handlers::ExternalBody{"Method not allowed"}
                );
        }
    } catch (const UserNotFoundError& e) {
        throw userver::server::handlers::ResourceNotFound(
            userver::server::handlers::ExternalBody{e.what()}
        );
    } catch (const ValidationError& e) {
        throw userver::server::handlers::ClientError(
            userver::server::handlers::ExternalBody{e.what()}
        );
    }
}

userver::engine::Task<std::string> UserProfileHandler::HandleGetProfile(
    const std::string& user_id) const {
    
    auto profile = co_await GetUserProfileAsync(user_id);
    
    // Convert to JSON response
    userver::formats::json::ValueBuilder response;
    response["user_id"] = profile.user_id;
    response["email"] = profile.email;
    response["name"] = profile.name;
    response["created_at"] = userver::utils::datetime::Timestring(profile.created_at);
    
    co_return ToString(response.ExtractValue());
}

} // namespace myservice::handlers
```

**Memory Bank Reference**: [`memory-bank://main/service-patterns#http-handlers`](../../memory-bank/main/service-patterns.md#http-handlers)

### Request Validation Patterns
Implement robust request validation:

```cpp
// Request validation utilities
namespace myservice::validation {

class RequestValidator {
public:
    static void ValidateUserCreateRequest(const userver::formats::json::Value& request) {
        // Required fields validation
        if (!request.HasMember("email") || request["email"].As<std::string>().empty()) {
            throw ValidationError("Email is required");
        }
        
        if (!request.HasMember("name") || request["name"].As<std::string>().empty()) {
            throw ValidationError("Name is required");
        }
        
        // Email format validation
        const auto email = request["email"].As<std::string>();
        if (!IsValidEmail(email)) {
            throw ValidationError("Invalid email format");
        }
        
        // Name length validation
        const auto name = request["name"].As<std::string>();
        if (name.length() < 2 || name.length() > 100) {
            throw ValidationError("Name must be between 2 and 100 characters");
        }
    }
    
    static void ValidateUserId(const std::string& user_id) {
        if (user_id.empty()) {
            throw ValidationError("User ID cannot be empty");
        }
        
        // UUID format validation
        if (!IsValidUuid(user_id)) {
            throw ValidationError("Invalid user ID format");
        }
    }

private:
    static bool IsValidEmail(const std::string& email) {
        // Simple email validation regex
        static const std::regex email_regex(
            R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
        );
        return std::regex_match(email, email_regex);
    }
    
    static bool IsValidUuid(const std::string& uuid) {
        static const std::regex uuid_regex(
            R"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})"
        );
        return std::regex_match(uuid, uuid_regex);
    }
};

} // namespace myservice::validation
```

## Database Integration Patterns

### Repository Pattern Implementation
Implement data access using repository pattern:

```cpp
#pragma once

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace myservice::repositories {

class UserRepository {
public:
    explicit UserRepository(userver::storages::postgres::ClusterPtr pg_cluster);
    
    userver::engine::Task<std::optional<User>> FindByIdAsync(const std::string& user_id) const;
    userver::engine::Task<std::optional<User>> FindByEmailAsync(const std::string& email) const;
    userver::engine::Task<User> CreateAsync(const CreateUserRequest& request) const;
    userver::engine::Task<User> UpdateAsync(const std::string& user_id, const UpdateUserRequest& request) const;
    userver::engine::Task<void> DeleteAsync(const std::string& user_id) const;
    userver::engine::Task<std::vector<User>> FindAllAsync(size_t limit, size_t offset) const;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
    static User MapRowToUser(const userver::storages::postgres::Row& row);
};

} // namespace myservice::repositories
```

```cpp
// Implementation
#include "user_repository.hpp"

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/uuid4.hpp>

namespace myservice::repositories {

UserRepository::UserRepository(userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {
}

userver::engine::Task<std::optional<User>> UserRepository::FindByIdAsync(
    const std::string& user_id) const {
    
    static const auto kQuery = R"(
        SELECT id, email, name, created_at, updated_at 
        FROM users 
        WHERE id = $1 AND deleted_at IS NULL
    )";
    
    try {
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kQuery, user_id
        );
        
        if (result.IsEmpty()) {
            co_return std::nullopt;
        }
        
        co_return MapRowToUser(result.AsSingleRow());
        
    } catch (const userver::storages::postgres::Error& e) {
        LOG_ERROR() << "Database error in FindByIdAsync: " << e.what()
                    << userver::logging::LogExtra{{"user_id", user_id}};
        throw DatabaseError("Failed to find user by ID");
    }
}

userver::engine::Task<User> UserRepository::CreateAsync(
    const CreateUserRequest& request) const {
    
    static const auto kQuery = R"(
        INSERT INTO users (id, email, name, created_at, updated_at)
        VALUES ($1, $2, $3, NOW(), NOW())
        RETURNING id, email, name, created_at, updated_at
    )";
    
    auto user_id = userver::utils::generators::GenerateUuid();
    
    try {
        auto result = co_await pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kQuery, user_id, request.email, request.name
        );
        
        co_return MapRowToUser(result.AsSingleRow());
        
    } catch (const userver::storages::postgres::UniqueViolation& e) {
        throw ValidationError("User with this email already exists");
    } catch (const userver::storages::postgres::Error& e) {
        LOG_ERROR() << "Database error in CreateAsync: " << e.what()
                    << userver::logging::LogExtra{{"email", request.email}};
        throw DatabaseError("Failed to create user");
    }
}

User UserRepository::MapRowToUser(const userver::storages::postgres::Row& row) {
    return User{
        .id = row["id"].As<std::string>(),
        .email = row["email"].As<std::string>(),
        .name = row["name"].As<std::string>(),
        .created_at = row["created_at"].As<std::chrono::system_clock::time_point>(),
        .updated_at = row["updated_at"].As<std::chrono::system_clock::time_point>()
    };
}

} // namespace myservice::repositories
```

### Transaction Management
Implement database transactions properly:

```cpp
// Service layer with transaction management
namespace myservice::services {

class UserService {
private:
    UserRepository user_repository_;
    AuditLogRepository audit_repository_;
    userver::storages::postgres::ClusterPtr pg_cluster_;
    
public:
    userver::engine::Task<User> CreateUserWithAuditAsync(
        const CreateUserRequest& request) const {
        
        // Start transaction
        auto transaction = co_await pg_cluster_->Begin(
            userver::storages::postgres::ClusterHostType::kMaster,
            userver::storages::postgres::TransactionOptions{}
        );
        
        try {
            // Create user within transaction
            auto user = co_await user_repository_.CreateAsync(transaction, request);
            
            // Create audit log entry
            AuditLogEntry audit_entry{
                .user_id = user.id,
                .action = "USER_CREATED",
                .details = fmt::format("User created with email: {}", user.email),
                .timestamp = std::chrono::system_clock::now()
            };
            
            co_await audit_repository_.CreateAsync(transaction, audit_entry);
            
            // Commit transaction
            co_await transaction.Commit();
            
            LOG_INFO() << "User created successfully"
                       << userver::logging::LogExtra{
                           {"user_id", user.id},
                           {"email", user.email}
                       };
            
            co_return user;
            
        } catch (const std::exception& e) {
            // Rollback on any error
            co_await transaction.Rollback();
            LOG_ERROR() << "Failed to create user, transaction rolled back: " << e.what();
            throw;
        }
    }
};

} // namespace myservice::services
```

**Memory Bank Reference**: [`memory-bank://main/service-patterns#database-patterns`](../../memory-bank/main/service-patterns.md#database-patterns)

## Caching Implementation Patterns

### Multi-Level Caching
Implement efficient caching strategies:

```cpp
namespace myservice::caching {

template<typename Key, typename Value>
class MultiLevelCache {
private:
    // L1: In-memory LRU cache
    userver::cache::LruCache<Key, Value> l1_cache_;
    
    // L2: Redis distributed cache
    userver::storages::redis::ClientPtr redis_client_;
    
    // L3: Database (source of truth)
    std::function<userver::engine::Task<std::optional<Value>>(const Key&)> data_loader_;
    
    std::chrono::seconds l1_ttl_;
    std::chrono::seconds l2_ttl_;
    
public:
    MultiLevelCache(size_t l1_capacity,
                   userver::storages::redis::ClientPtr redis_client,
                   std::function<userver::engine::Task<std::optional<Value>>(const Key&)> data_loader,
                   std::chrono::seconds l1_ttl = std::chrono::seconds{300},
                   std::chrono::seconds l2_ttl = std::chrono::seconds{3600})
        : l1_cache_(l1_capacity),
          redis_client_(std::move(redis_client)),
          data_loader_(std::move(data_loader)),
          l1_ttl_(l1_ttl),
          l2_ttl_(l2_ttl) {
    }
    
    userver::engine::Task<std::optional<Value>> GetAsync(const Key& key) const {
        auto cache_key = SerializeKey(key);
        
        // Try L1 cache first
        if (auto cached = l1_cache_.Get(cache_key)) {
            co_return *cached;
        }
        
        // Try L2 cache (Redis)
        auto redis_value = co_await redis_client_->GetAsync(cache_key);
        if (redis_value) {
            auto value = DeserializeValue(*redis_value);
            
            // Populate L1 cache
            l1_cache_.Put(cache_key, value);
            
            co_return value;
        }
        
        // Load from source (L3)
        auto value = co_await data_loader_(key);
        if (!value) {
            co_return std::nullopt;
        }
        
        // Populate both cache levels
        co_await redis_client_->SetAsync(cache_key, SerializeValue(*value), l2_ttl_);
        l1_cache_.Put(cache_key, *value);
        
        co_return value;
    }
    
    userver::engine::Task<void> InvalidateAsync(const Key& key) const {
        auto cache_key = SerializeKey(key);
        
        // Invalidate both levels
        l1_cache_.Invalidate(cache_key);
        co_await redis_client_->DelAsync(cache_key);
    }

private:
    std::string SerializeKey(const Key& key) const;
    std::string SerializeValue(const Value& value) const;
    Value DeserializeValue(const std::string& data) const;
};

// Usage example
class UserProfileCache {
private:
    MultiLevelCache<std::string, UserProfile> cache_;
    
public:
    UserProfileCache(userver::storages::redis::ClientPtr redis_client,
                    UserRepository& user_repository)
        : cache_(1000,  // L1 capacity
                std::move(redis_client),
                [&user_repository](const std::string& user_id) -> userver::engine::Task<std::optional<UserProfile>> {
                    auto user = co_await user_repository.FindByIdAsync(user_id);
                    if (!user) {
                        co_return std::nullopt;
                    }
                    co_return UserProfile::FromUser(*user);
                }) {
    }
    
    userver::engine::Task<std::optional<UserProfile>> GetUserProfileAsync(
        const std::string& user_id) const {
        return cache_.GetAsync(user_id);
    }
    
    userver::engine::Task<void> InvalidateUserProfileAsync(
        const std::string& user_id) const {
        return cache_.InvalidateAsync(user_id);
    }
};

} // namespace myservice::caching
```

## Error Handling Implementation

### Structured Error Handling
Implement comprehensive error handling:

```cpp
namespace myservice::errors {

// Base exception hierarchy
class ServiceError : public std::exception {
private:
    std::string message_;
    std::string error_code_;
    
public:
    ServiceError(std::string message, std::string error_code = "INTERNAL_ERROR")
        : message_(std::move(message)), error_code_(std::move(error_code)) {
    }
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    const std::string& GetErrorCode() const noexcept {
        return error_code_;
    }
};

class ValidationError : public ServiceError {
public:
    explicit ValidationError(const std::string& message)
        : ServiceError(message, "VALIDATION_ERROR") {
    }
};

class UserNotFoundError : public ServiceError {
public:
    explicit UserNotFoundError(const std::string& user_id)
        : ServiceError(fmt::format("User not found: {}", user_id), "USER_NOT_FOUND") {
    }
};

class DatabaseError : public ServiceError {
public:
    explicit DatabaseError(const std::string& message)
        : ServiceError(message, "DATABASE_ERROR") {
    }
};

// Error response formatter
class ErrorResponseFormatter {
public:
    static std::string FormatErrorResponse(const ServiceError& error) {
        userver::formats::json::ValueBuilder response;
        response["error"]["code"] = error.GetErrorCode();
        response["error"]["message"] = error.what();
        response["error"]["timestamp"] = userver::utils::datetime::Now();
        
        return ToString(response.ExtractValue());
    }
    
    static userver::server::handlers::HandlerErrorCode MapToHttpStatus(
        const ServiceError& error) {
        
        if (dynamic_cast<const ValidationError*>(&error)) {
            return userver::server::handlers::HandlerErrorCode::kClientError;
        }
        
        if (dynamic_cast<const UserNotFoundError*>(&error)) {
            return userver::server::handlers::HandlerErrorCode::kResourceNotFound;
        }
        
        return userver::server::handlers::HandlerErrorCode::kServerSideError;
    }
};

} // namespace myservice::errors

// Usage in handlers
std::string UserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    
    try {
        return ProcessRequest(request).Get();
    } catch (const myservice::errors::ServiceError& e) {
        // Log error with context
        LOG_ERROR() << "Service error: " << e.what()
                    << userver::logging::LogExtra{
                        {"error_code", e.GetErrorCode()},
                        {"request_path", request.GetUrl()},
                        {"user_agent", request.GetHeader("User-Agent")}
                    };
        
        // Set appropriate HTTP status
        auto status = myservice::errors::ErrorResponseFormatter::MapToHttpStatus(e);
        request.GetHttpResponse().SetStatus(status);
        
        // Return formatted error response
        return myservice::errors::ErrorResponseFormatter::FormatErrorResponse(e);
    }
}
```

**Cross-Reference**: [`pattern://error_handling/structured_exceptions`](../../memory-bank/main/framework-core.md#error-handling)

## Testing Implementation Patterns

### Unit Testing Components
Implement comprehensive unit tests:

```cpp
#include <gtest/gtest.h>
#include <userver/utest/utest.hpp>

namespace myservice::tests {

class UserServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test database
        pg_cluster_ = CreateTestPostgresCluster();
        redis_client_ = CreateTestRedisClient();
        
        user_repository_ = std::make_unique<UserRepository>(pg_cluster_);
        user_service_ = std::make_unique<UserService>(*user_repository_, redis_client_);
    }
    
    void TearDown() override {
        // Cleanup test data
        CleanupTestDatabase();
    }
    
    userver::storages::postgres::ClusterPtr pg_cluster_;
    userver::storages::redis::ClientPtr redis_client_;
    std::unique_ptr<UserRepository> user_repository_;
    std::unique_ptr<UserService> user_service_;
};

UTEST_F(UserServiceTest, CreateUser_ValidRequest_Success) {
    CreateUserRequest request{
        .email = "test@example.com",
        .name = "Test User"
    };
    
    auto user = user_service_->CreateUserAsync(request).Get();
    
    EXPECT_FALSE(user.id.empty());
    EXPECT_EQ(user.email, request.email);
    EXPECT_EQ(user.name, request.name);
}

UTEST_F(UserServiceTest, CreateUser_DuplicateEmail_ThrowsValidationError) {
    CreateUserRequest request{
        .email = "duplicate@example.com",
        .name = "Test User"
    };
    
    // Create first user
    user_service_->CreateUserAsync(request).Get();
    
    // Attempt to create duplicate
    EXPECT_THROW(
        user_service_->CreateUserAsync(request).Get(),
        ValidationError
    );
}

UTEST_F(UserServiceTest, GetUser_ExistingUser_ReturnsUser) {
    // Setup: Create test user
    auto created_user = CreateTestUser();
    
    // Test: Get user
    auto retrieved_user = user_service_->GetUserAsync(created_user.id).Get();
    
    ASSERT_TRUE(retrieved_user.has_value());
    EXPECT_EQ(retrieved_user->id, created_user.id);
    EXPECT_EQ(retrieved_user->email, created_user.email);
}

UTEST_F(UserServiceTest, GetUser_NonExistentUser_ReturnsNullopt) {
    auto result = user_service_->GetUserAsync("non-existent-id").Get();
    EXPECT_FALSE(result.has_value());
}

} // namespace myservice::tests
```

### Integration Testing
Implement integration tests with real components:

```cpp
namespace myservice::integration_tests {

class UserHandlerIntegrationTest : public userver::utest::HttpHandlerTest {
protected:
    void SetUp() override {
        // Setup test environment with real components
        auto component_list = userver::components::MinimalServerComponentList()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::components::Postgres>("postgres-test")
            .Append<userver::components::Redis>("redis-test")
            .Append<UserRepository>()
            .Append<UserService>()
            .Append<UserHandler>();
        
        SetUpComponents(component_list);
    }
};

UTEST_F(UserHandlerIntegrationTest, CreateUser_ValidRequest_Returns201) {
    auto request = userver::formats::json::ValueBuilder{};
    request["email"] = "integration@example.com";
    request["name"] = "Integration Test User";
    
    auto response = PostRequest("/users", ToString(request.ExtractValue()));
    
    EXPECT_EQ(response.status, 201);
    
    auto response_json = userver::formats::json::FromString(response.body);
    EXPECT_FALSE(response_json["id"].As<std::string>().empty());
    EXPECT_EQ(response_json["email"].As<std::string>(), "integration@example.com");
}

UTEST_F(UserHandlerIntegrationTest, GetUser_ExistingUser_Returns200) {
    // Setup: Create test user
    auto user = CreateTestUserViaApi();
    
    // Test: Get user
    auto response = GetRequest(fmt::format("/users/{}", user.id));
    
    EXPECT_EQ(response.status, 200);
    
    auto response_json = userver::formats::json::FromString(response.body);
    EXPECT_EQ(response_json["id"].As<std::string>(), user.id);
}

} // namespace myservice::integration_tests
```

**Memory Bank Reference**: [`memory-bank://main/service-patterns#testing-patterns`](../../memory-bank/main/service-patterns.md#testing-patterns)

## Cross-References

### Related Memory Bank Entries
- [`memory-bank://main/service-patterns`](../../memory-bank/main/service-patterns.md) - Service implementation patterns
- [`memory-bank://main/framework-core`](../../memory-bank/main/framework-core.md) - Core framework usage
- [`memory-bank://main/async-programming`](../../memory-bank/main/async-programming.md) - Async implementation details

### Implementation Examples
- [`example://handlers/crud_operations`](../../../samples/postgres_service/) - CRUD handler implementation
- [`example://caching/multi_level`](../../../samples/cache_service/) - Caching implementation
- [`example://testing/comprehensive`](../../../samples/hello_service/unittests/) - Testing patterns

### Alternative Approaches
- [`alternative://repository_vs_active_record`](../../memory-bank/research/new-patterns.md#data-access-patterns) - Data access patterns
- [`alternative://sync_vs_async_handlers`](../../memory-bank/main/troubleshooting-guide.md#handler-patterns) - Handler implementation choices

---

**Mode Context**: Code mode focuses on concrete implementation patterns and best practices.  
**Inheritance**: Extends global rules with implementation-specific guidance.  
**Dependencies**: [`global.framework.fundamentals`](../../00-global/framework-fundamentals.md), [`global.component.system`](../../00-global/component-system.md), [`global.async.programming`](../../00-global/async-programming.md)  
**Last Updated**: 2025-01-05  
**Next Review**: 2025-04-05