# Service Development Patterns in Userver

## Overview

This document outlines the established patterns and best practices for developing services using the userver framework. These patterns help ensure consistency, maintainability, and scalability across different services.

## Service Structure

### Standard Directory Layout

```
service-name/
├── src/
│   ├── components/          # Custom components
│   ├── handlers/            # HTTP handlers
│   ├── models/              # Data models and business logic
│   ├── views/               # Response formatting
│   └── service.cpp          # Main service entry point
├── configs/
│   ├── config.yaml          # Configuration files
│   └── static/              # Static configuration data
├── tests/
│   ├── unit/                # Unit tests
│   ├── functional/          # Functional tests
│   └── integration/         # Integration tests
├── CMakeLists.txt           # Build configuration
└── README.md                # Service documentation
```

### Component Organization

#### Handler Components
```cpp
// handlers/ping.hpp
#pragma once

#include <userver/server/handlers/http_handler_base.hpp>

namespace handlers {

class Ping final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-ping";
    
    Ping(const components::ComponentConfig& config,
         const components::ComponentContext& context);
    
    std::string HandleRequestThrow(
        const server::http::HttpRequest& request,
        server::request::RequestContext& context) const override;
};

} // namespace handlers
```

#### Business Logic Components
```cpp
// components/user_service.hpp
#pragma once

#include <userver/components/component.hpp>

namespace components {

class UserService final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "user-service";
    
    UserService(const components::ComponentConfig& config,
                const components::ComponentContext& context);
    
    // Business logic methods
    models::User GetUser(UserId id) const;
    void CreateUser(const models::User& user);
    
private:
    // Dependencies
    storages::postgres::ClusterPtr pg_cluster_;
};

} // namespace components
```

## HTTP Handler Patterns

### Request Handling

#### Basic Handler Structure
```cpp
std::string Ping::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& context) const {
    
    // Request validation
    if (request.GetMethod() != server::http::HttpMethod::kGet) {
        request.GetHttpResponse().SetStatus(server::http::HttpStatus::kMethodNotAllowed);
        return {};
    }
    
    // Business logic
    auto result = DoPing();
    
    // Response formatting
    return formats::json::ValueBuilder(result).ExtractValue().ToString();
}
```

#### Request Parameter Handling
```cpp
void ValidateRequest(const server::http::HttpRequest& request) {
    // Path parameters
    auto user_id = request.GetPathArg("user_id");
    
    // Query parameters
    auto limit = request.GetArg("limit");
    
    // Request body
    auto body = request.RequestBody();
    
    // Headers
    auto auth_header = request.GetHeader("Authorization");
    
    // Validation
    if (user_id.empty()) {
        throw server::handlers::ClientError(
            server::handlers::ExternalBody{"Missing user_id"}
        );
    }
}
```

### Response Patterns

#### Success Responses
```cpp
formats::json::Value BuildSuccessResponse(const models::User& user) {
    formats::json::ValueBuilder builder;
    builder["status"] = "success";
    builder["data"] = user.ToJson();
    return builder.ExtractValue();
}
```

#### Error Responses
```cpp
formats::json::Value BuildErrorResponse(
    server::http::HttpStatus status,
    const std::string& message,
    const std::string& error_code = "") {
    
    formats::json::ValueBuilder builder;
    builder["status"] = "error";
    builder["message"] = message;
    
    if (!error_code.empty()) {
        builder["error_code"] = error_code;
    }
    
    auto response = builder.ExtractValue();
    throw server::handlers::CustomHandlerException(
        response.ToString(),
        status
    );
}
```

## Business Logic Patterns

### Service Layer Pattern

```cpp
class UserService {
public:
    UserService(storages::postgres::ClusterPtr cluster);
    
    models::User GetUser(UserId id) const;
    models::User CreateUser(const models::CreateUserRequest& request);
    void UpdateUser(UserId id, const models::UpdateUserRequest& request);
    void DeleteUser(UserId id);
    
private:
    storages::postgres::ClusterPtr cluster_;
    
    // Helper methods
    bool UserExists(UserId id) const;
    void ValidateUser(const models::User& user) const;
};
```

### Repository Pattern

```cpp
class UserRepository {
public:
    UserRepository(storages::postgres::ClusterPtr cluster);
    
    std::optional<models::User> FindById(UserId id) const;
    std::vector<models::User> FindAll(int limit, int offset) const;
    models::User Insert(const models::User& user);
    void Update(UserId id, const models::UpdateUserRequest& request);
    void Delete(UserId id);
    
private:
    storages::postgres::ClusterPtr cluster_;
    
    static constexpr storages::postgres::Query kSelectById{
        "SELECT id, name, email FROM users WHERE id = $1",
        storages::postgres::Query::Name{"select_user_by_id"}
    };
};
```

## Data Model Patterns

### Model Definition

```cpp
struct User {
    UserId id;
    std::string name;
    std::string email;
    std::chrono::system_clock::time_point created_at;
    
    // Serialization
    formats::json::Value ToJson() const;
    static User FromJson(const formats::json::Value& json);
    
    // Validation
    void Validate() const;
    
    // Comparison
    bool operator==(const User& other) const = default;
};
```

### Type Safety

```cpp
// Strongly typed IDs
class UserId {
public:
    explicit UserId(std::int64_t value) : value_(value) {}
    
    std::int64_t GetValue() const { return value_; }
    
    bool operator==(const UserId& other) const = default;
    
private:
    std::int64_t value_;
};

// Type aliases for clarity
using Email = std::string;
using Username = std::string;
```

## Configuration Patterns

### Component Configuration

```cpp
struct UserServiceConfig {
    std::chrono::milliseconds timeout{std::chrono::seconds(5)};
    int max_retries{3};
    std::string default_role{"user"};
    
    static UserServiceConfig Parse(const yaml_config::YamlConfig& config,
                                  formats::parse::To<UserServiceConfig>);
};

void Append(builder, value) {
    builder["timeout-ms"] = value.timeout.count();
    builder["max-retries"] = value.max_retries;
    builder["default-role"] = value.default_role;
}

UserServiceConfig Parse(yaml_config::YamlConfig value,
                       formats::parse::To<UserServiceConfig>) {
    UserServiceConfig config;
    config.timeout = std::chrono::milliseconds{
        value["timeout-ms"].As<int>()
    };
    config.max_retries = value["max-retries"].As<int>();
    config.default_role = value["default-role"].As<std::string>();
    return config;
}
```

### Dynamic Configuration

```cpp
class UserService {
public:
    UserService(const components::ComponentConfig& config,
                const components::ComponentContext& context)
        : config_source_(
            context.FindComponent<components::DynamicConfig>().GetSource()) {
    }
    
    void DoSomething() {
        auto config = config_source_();
        auto timeout = config[kUserServiceTimeout].As<std::chrono::milliseconds>();
        // Use timeout
    }
    
private:
    dynamic_config::Source config_source_;
};

// Configuration schema
constexpr dynamic_config::Key kUserServiceTimeout{
    "USER_SERVICE_TIMEOUT",
    std::chrono::milliseconds{5000}
};
```

## Error Handling Patterns

### Custom Exception Types

```cpp
class UserNotFoundException : public std::exception {
public:
    explicit UserNotFoundException(UserId user_id)
        : user_id_(user_id) {}
    
    const char* what() const noexcept override {
        return "User not found";
    }
    
    UserId GetUserId() const { return user_id_; }
    
private:
    UserId user_id_;
};
```

### Error Mapping

```cpp
server::handlers::HandlerErrorCode MapToHandlerError(
    const std::exception& ex) {
    
    if (dynamic_cast<const UserNotFoundException*>(&ex)) {
        return server::handlers::HandlerErrorCode::kNotFound;
    }
    
    if (dynamic_cast<const ValidationException*>(&ex)) {
        return server::handlers::HandlerErrorCode::kBadRequest;
    }
    
    return server::handlers::HandlerErrorCode::kInternalServerError;
}
```

## Testing Patterns

### Unit Testing

```cpp
class UserServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_cluster_ = std::make_shared<storages::postgres::MockCluster>();
        user_service_ = std::make_unique<UserService>(mock_cluster_);
    }
    
    std::shared_ptr<storages::postgres::MockCluster> mock_cluster_;
    std::unique_ptr<UserService> user_service_;
};

TEST_F(UserServiceTest, GetUserSuccess) {
    // Setup mock
    EXPECT_CALL(*mock_cluster_, Execute(_, _, _))
        .WillOnce(Return(CreateMockResult()));
    
    // Test
    auto user = user_service_->GetUser(UserId{123});
    
    // Verify
    EXPECT_EQ(user.id.GetValue(), 123);
}
```

### Integration Testing

```python
# testsuite test
async def test_get_user(service_client):
    response = await service_client.get('/v1/users/123')
    assert response.status == 200
    data = response.json()
    assert data['id'] == 123
```

## Performance Patterns

### Caching

```cpp
class UserService {
public:
    UserService(cache::CacheContainer& cache_container)
        : user_cache_(cache_container.GetCache("user-cache")) {}
    
    models::User GetUser(UserId id) const {
        // Try cache first
        if (auto cached = user_cache_->Get(id)) {
            return *cached;
        }
        
        // Load from database
        auto user = LoadUserFromDatabase(id);
        
        // Store in cache
        user_cache_->Put(id, user);
        
        return user;
    }
    
private:
    cache::LruCache<UserId, models::User> user_cache_;
};
```

### Connection Pooling

```cpp
// Use cluster for connection pooling
auto cluster = context.FindComponent<storages::postgres::Cluster>();

// Execute queries through cluster
auto result = cluster->Execute(
    storages::postgres::ClusterHostType::kMaster,
    query,
    params...
);
```

### Batch Operations

```cpp
std::vector<models::User> UserService::GetUsers(
    const std::vector<UserId>& user_ids) const {
    
    if (user_ids.empty()) {
        return {};
    }
    
    // Batch query instead of individual queries
    auto query = storages::postgres::Query::FromStrings(
        "SELECT id, name, email FROM users WHERE id = ANY($1)",
        {formats::postgres::ToPgArray(user_ids)}
    );
    
    auto result = cluster_->Execute(
        storages::postgres::ClusterHostType::kSlave,
        query
    );
    
    std::vector<models::User> users;
    for (auto row : result) {
        users.push_back(models::User::FromRow(row));
    }
    
    return users;
}
```

## Security Patterns

### Authentication

```cpp
class AuthMiddleware : public server::handlers::HttpMiddlewareBase {
public:
    AuthMiddleware(const components::ComponentContext& context)
        : user_service_(context.FindComponent<components::UserService>()) {}
    
    void HandleRequest(
        server::http::HttpRequest& request,
        server::request::RequestContext& context) const override {
        
        auto auth_header = request.GetHeader("Authorization");
        if (auth_header.empty()) {
            throw server::handlers::ClientError(
                server::handlers::ExternalBody{"Missing Authorization header"}
            );
        }
        
        auto user = ValidateToken(auth_header);
        context.SetUserData("current_user", user);
        
        call_next_(request, context);
    }
    
private:
    components::UserService& user_service_;
};
```

### Input Validation

```cpp
void ValidateUser(const models::CreateUserRequest& request) {
    if (request.email.empty()) {
        throw ValidationException("Email is required");
    }
    
    if (!IsValidEmail(request.email)) {
        throw ValidationException("Invalid email format");
    }
    
    if (request.name.length() > 100) {
        throw ValidationException("Name too long");
    }
}
```

These patterns provide a solid foundation for building robust, maintainable, and scalable services using the userver framework. Following these established patterns helps ensure consistency across different services and makes it easier for developers to understand and work with the codebase.