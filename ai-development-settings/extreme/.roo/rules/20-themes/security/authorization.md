# Authorization Patterns

## Overview

Comprehensive authorization patterns for userver applications, covering Role-Based Access Control (RBAC), Attribute-Based Access Control (ABAC), permission management, resource-level authorization, and API endpoint protection.

## Core Authorization Components

### Role-Based Access Control (RBAC)
- [`server::auth::UserScope`](https://userver.tech/d4/d67/classutils_1_1StrongTypedef.html) - Strongly typed user scopes/roles
- [`server::handlers::auth::AuthCheckerBase`](https://userver.tech/d6/db7/classserver_1_1handlers_1_1auth_1_1AuthCheckerBase.html) - Base class for authorization checkers
- [`server::request::RequestContext`](https://userver.tech/de/df6/classserver_1_1request_1_1RequestContext.html) - Request context for storing authorization data

### Attribute-Based Access Control (ABAC)
- [`formats::json::Value`](https://userver.tech/d2/d20/md_en_2userver_2formats.html) - JSON value handling for policy definitions
- [`concurrent::Variable`](https://userver.tech/d8/dcc/namespaceconcurrent.html) - Thread-safe policy storage
- [`crypto::algorithm`](https://userver.tech/de/d55/algorithm_8hpp.html) - Cryptographic algorithms for secure comparisons

### Permission Management
- [`components::PostgreCache`](https://userver.tech/d2/d8d/classcomponents_1_1PostgreCache.html) - Caching authorization policies
- [`server::auth::UserAuthInfo`](https://userver.tech/d4/d67/classutils_1_1StrongTypedef.html) - User authentication information
- [`utils::Async`](https://userver.tech/dc/db7/group__userver__concurrency.html#ga29b9a09eacda1ddf8b4440b3713f5e52) - Asynchronous policy updates

## Role-Based Access Control (RBAC) Implementation

### RBAC Core Components
```cpp
#include <userver/server/auth/user_auth_info.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json.hpp>

// Role definitions
enum class UserRole {
  kAdmin,
  kUser,
  kModerator,
  kGuest
};

// Permission definitions
enum class Permission {
  kRead,
  kWrite,
  kDelete,
  kAdminAccess,
  kModerateContent
};

// Role-Permission mapping
class RolePermissionManager {
public:
  RolePermissionManager() {
    // Initialize role-permission mappings
    role_permissions_[UserRole::kAdmin] = {
      Permission::kRead, Permission::kWrite, Permission::kDelete,
      Permission::kAdminAccess, Permission::kModerateContent
    };
    
    role_permissions_[UserRole::kUser] = {
      Permission::kRead, Permission::kWrite
    };
    
    role_permissions_[UserRole::kModerator] = {
      Permission::kRead, Permission::kWrite, Permission::kModerateContent
    };
    
    role_permissions_[UserRole::kGuest] = {
      Permission::kRead
    };
  }
  
  bool HasPermission(UserRole role, Permission permission) const {
    auto it = role_permissions_.find(role);
    if (it == role_permissions_.end()) {
      return false;
    }
    
    const auto& permissions = it->second;
    return std::find(permissions.begin(), permissions.end(), permission) != 
           permissions.end();
  }
  
  std::vector<Permission> GetPermissions(UserRole role) const {
    auto it = role_permissions_.find(role);
    if (it != role_permissions_.end()) {
      return it->second;
    }
    return {};
  }

private:
  std::unordered_map<UserRole, std::vector<Permission>> role_permissions_;
};

// User-Role mapping with caching
struct UserRoles {
  std::vector<UserRole> roles;
  std::chrono::system_clock::time_point last_updated;
  std::chrono::seconds cache_ttl{300}; // 5 minutes
};

class UserRoleCache {
public:
  UserRoleCache(const components::ComponentConfig& config,
                const components::ComponentContext& context)
    : cache_ttl_(config["cache-ttl"].As<int>(300)) {
    // Initialize database connection or other dependencies
  }
  
  std::vector<UserRole> GetUserRoles(const std::string& user_id) {
    auto cache = user_roles_cache_.Lock();
    
    auto it = cache->find(user_id);
    if (it != cache->end()) {
      auto& user_roles = it->second;
      auto now = std::chrono::system_clock::now();
      
      // Check if cache is still valid
      if (now - user_roles.last_updated < user_roles.cache_ttl) {
        return user_roles.roles;
      }
    }
    
    // Cache miss or expired - fetch from database
    auto roles = FetchUserRolesFromDatabase(user_id);
    
    // Update cache
    UserRoles user_roles;
    user_roles.roles = roles;
    user_roles.last_updated = std::chrono::system_clock::now();
    user_roles.cache_ttl = cache_ttl_;
    
    (*cache)[user_id] = std::move(user_roles);
    
    return roles;
  }
  
  void InvalidateUserRoles(const std::string& user_id) {
    auto cache = user_roles_cache_.Lock();
    cache->erase(user_id);
  }

private:
  std::vector<UserRole> FetchUserRolesFromDatabase(const std::string& user_id) {
    // Implementation to fetch user roles from database
    // This is a simplified example
    return {UserRole::kUser};
  }
  
  concurrent::Variable<std::unordered_map<std::string, UserRoles>> user_roles_cache_;
  std::chrono::seconds cache_ttl_;
};
```

### RBAC Authorization Checker
```cpp
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/auth/user_auth_info.hpp>

class RbacAuthChecker : public server::handlers::auth::AuthCheckerBase {
public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;
  
  RbacAuthChecker(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
    : AuthCheckerBase(config, context),
      role_permission_manager_(std::make_unique<RolePermissionManager>()),
      user_role_cache_(std::make_unique<UserRoleCache>(config, context)) {
    
    // Parse required roles from configuration
    auto required_roles_config = config["required-roles"];
    for (const auto& role : required_roles_config.As<std::vector<std::string>>()) {
      required_roles_.push_back(ParseUserRole(role));
    }
  }
  
  AuthCheckResult CheckAuth(const server::http::HttpRequest& request,
                           server::request::RequestContext& context) const override {
    try {
      // Get user ID from authentication context
      auto user_id_opt = context.GetData<std::string>("user_id");
      if (!user_id_opt.has_value()) {
        return AuthCheckResult::kUserNotFound;
      }
      
      auto user_id = user_id_opt.value();
      
      // Get user roles
      auto user_roles = user_role_cache_->GetUserRoles(user_id);
      
      // Check if user has any of the required roles
      bool has_required_role = false;
      for (const auto& required_role : required_roles_) {
        if (std::find(user_roles.begin(), user_roles.end(), required_role) != 
            user_roles.end()) {
          has_required_role = true;
          break;
        }
      }
      
      if (!has_required_role) {
        return AuthCheckResult::kForbidden;
      }
      
      // Store user roles in context for further authorization checks
      context.SetData("user_roles", user_roles);
      
      return AuthCheckResult::kOk;
      
    } catch (const std::exception& e) {
      LOG_WARNING() << "RBAC authorization failed: " << e.what();
      return AuthCheckResult::kForbidden;
    }
  }
  
  bool SupportsUserAuth() const noexcept override { return true; }

private:
  UserRole ParseUserRole(const std::string& role_str) const {
    if (role_str == "admin") return UserRole::kAdmin;
    if (role_str == "user") return UserRole::kUser;
    if (role_str == "moderator") return UserRole::kModerator;
    if (role_str == "guest") return UserRole::kGuest;
    
    throw std::invalid_argument("Unknown role: " + role_str);
  }
  
  std::unique_ptr<RolePermissionManager> role_permission_manager_;
  std::unique_ptr<UserRoleCache> user_role_cache_;
  std::vector<UserRole> required_roles_;
};
```

## Attribute-Based Access Control (ABAC) Implementation

### ABAC Policy Engine
```cpp
#include <userver/formats/json.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/crypto/algorithm.hpp>

// Policy definition
struct AbacPolicy {
  std::string id;
  std::string description;
  std::unordered_map<std::string, std::string> conditions;
  std::vector<std::string> permissions;
  std::chrono::system_clock::time_point created_at;
  bool is_active = true;
};

// Resource definition
struct Resource {
  std::string type;
  std::string id;
  std::unordered_map<std::string, std::string> attributes;
};

// Subject definition (user or service)
struct Subject {
  std::string id;
  std::string type; // user, service, etc.
  std::unordered_map<std::string, std::string> attributes;
};

class AbacPolicyEngine {
public:
  AbacPolicyEngine(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : policy_cache_ttl_(config["policy-cache-ttl"].As<int>(300)) {}
  
  bool CheckAccess(const Subject& subject, 
                   const Resource& resource, 
                   const std::string& action) {
    // Get applicable policies
    auto policies = GetApplicablePolicies(subject, resource, action);
    
    // Evaluate policies
    for (const auto& policy : policies) {
      if (EvaluatePolicy(policy, subject, resource, action)) {
        return true;
      }
    }
    
    return false;
  }
  
  void AddPolicy(const AbacPolicy& policy) {
    auto policies = policies_.Lock();
    (*policies)[policy.id] = policy;
    
    // Invalidate cache for this resource type
    InvalidatePolicyCache(policy);
  }
  
  void RemovePolicy(const std::string& policy_id) {
    auto policies = policies_.Lock();
    policies->erase(policy_id);
    
    // Invalidate related caches
    InvalidateAllCaches();
  }

private:
  std::vector<AbacPolicy> GetApplicablePolicies(const Subject& subject,
                                               const Resource& resource,
                                               const std::string& action) {
    // Check cache first
    std::string cache_key = GenerateCacheKey(subject, resource, action);
    auto cache = policy_cache_.Lock();
    
    auto it = cache->find(cache_key);
    if (it != cache->end()) {
      auto& cached_result = it->second;
      auto now = std::chrono::system_clock::now();
      
      if (now - cached_result.timestamp < policy_cache_ttl_) {
        return cached_result.policies;
      }
    }
    
    // Cache miss or expired - evaluate all policies
    std::vector<AbacPolicy> applicable_policies;
    
    auto policies = policies_.Lock();
    for (const auto& [id, policy] : *policies) {
      if (!policy.is_active) continue;
      
      if (IsPolicyApplicable(policy, subject, resource, action)) {
        applicable_policies.push_back(policy);
      }
    }
    
    // Cache the result
    CachedPolicyResult result;
    result.policies = applicable_policies;
    result.timestamp = std::chrono::system_clock::now();
    
    (*cache)[cache_key] = std::move(result);
    
    return applicable_policies;
  }
  
  bool EvaluatePolicy(const AbacPolicy& policy,
                     const Subject& subject,
                     const Resource& resource,
                     const std::string& action) const {
    // Evaluate all conditions
    for (const auto& [attribute, expected_value] : policy.conditions) {
      std::string actual_value;
      
      // Check subject attributes
      auto subject_it = subject.attributes.find(attribute);
      if (subject_it != subject.attributes.end()) {
        actual_value = subject_it->second;
      } else {
        // Check resource attributes
        auto resource_it = resource.attributes.find(attribute);
        if (resource_it != resource.attributes.end()) {
          actual_value = resource_it->second;
        } else {
          // Attribute not found - policy doesn't apply
          return false;
        }
      }
      
      // Check condition (simple equality for this example)
      if (actual_value != expected_value) {
        return false;
      }
    }
    
    // All conditions met - check if action is permitted
    return std::find(policy.permissions.begin(), policy.permissions.end(), action) != 
           policy.permissions.end();
  }
  
  bool IsPolicyApplicable(const AbacPolicy& policy,
                         const Subject& subject,
                         const Resource& resource,
                         const std::string& action) const {
    // Simple applicability check - in a real implementation, this would be more complex
    return true;
  }
  
  std::string GenerateCacheKey(const Subject& subject,
                              const Resource& resource,
                              const std::string& action) const {
    return subject.id + ":" + resource.type + ":" + resource.id + ":" + action;
  }
  
  void InvalidatePolicyCache(const AbacPolicy& policy) {
    // Invalidate cache entries related to this policy
    auto cache = policy_cache_.Lock();
    // Implementation depends on policy structure
  }
  
  void InvalidateAllCaches() {
    auto cache = policy_cache_.Lock();
    cache->clear();
  }
  
  struct CachedPolicyResult {
    std::vector<AbacPolicy> policies;
    std::chrono::system_clock::time_point timestamp;
  };
  
  concurrent::Variable<std::unordered_map<std::string, AbacPolicy>> policies_;
  concurrent::Variable<std::unordered_map<std::string, CachedPolicyResult>> policy_cache_;
  std::chrono::seconds policy_cache_ttl_;
};

// ABAC Authorization Checker
class AbacAuthChecker : public server::handlers::auth::AuthCheckerBase {
public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;
  
  AbacAuthChecker(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
    : AuthCheckerBase(config, context),
      policy_engine_(std::make_unique<AbacPolicyEngine>(config, context)) {}
  
  AuthCheckResult CheckAuth(const server::http::HttpRequest& request,
                           server::request::RequestContext& context) const override {
    try {
      // Get subject information from authentication context
      Subject subject;
      auto user_id_opt = context.GetData<std::string>("user_id");
      if (!user_id_opt.has_value()) {
        return AuthCheckResult::kUserNotFound;
      }
      
      subject.id = user_id_opt.value();
      subject.type = "user";
      
      // Get subject attributes (roles, groups, etc.)
      auto user_roles_opt = context.GetData<std::vector<std::string>>("user_roles");
      if (user_roles_opt.has_value()) {
        subject.attributes["roles"] = JoinStrings(user_roles_opt.value(), ",");
      }
      
      // Get resource information from request
      Resource resource;
      resource.type = "api_endpoint";
      resource.id = std::string(request.GetUrl());
      
      // Add resource attributes
      resource.attributes["method"] = ToString(request.GetMethod());
      resource.attributes["path"] = request.GetPath();
      
      // Get action from HTTP method
      std::string action = GetActionFromMethod(request.GetMethod());
      
      // Check access
      if (!policy_engine_->CheckAccess(subject, resource, action)) {
        return AuthCheckResult::kForbidden;
      }
      
      return AuthCheckResult::kOk;
      
    } catch (const std::exception& e) {
      LOG_WARNING() << "ABAC authorization failed: " << e.what();
      return AuthCheckResult::kForbidden;
    }
  }
  
  bool SupportsUserAuth() const noexcept override { return true; }

private:
  std::string GetActionFromMethod(server::http::HttpMethod method) const {
    switch (method) {
      case server::http::HttpMethod::kGet: return "read";
      case server::http::HttpMethod::kPost: return "create";
      case server::http::HttpMethod::kPut: return "update";
      case server::http::HttpMethod::kDelete: return "delete";
      case server::http::HttpMethod::kPatch: return "update";
      default: return "unknown";
    }
  }
  
  std::string JoinStrings(const std::vector<std::string>& strings, 
                         const std::string& delimiter) const {
    if (strings.empty()) return "";
    
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
      result += delimiter + strings[i];
    }
    return result;
  }
  
  std::unique_ptr<AbacPolicyEngine> policy_engine_;
};
```

## Resource-Level Authorization Implementation

### Resource Ownership and Access Control
```cpp
#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>

// Resource ownership information
struct ResourceOwnership {
  std::string resource_id;
  std::string resource_type;
  std::string owner_id;
  std::vector<std::string> shared_with; // User IDs with access
  std::unordered_map<std::string, std::vector<std::string>> permissions; // user_id -> permissions
  std::chrono::system_clock::time_point created_at;
};

class ResourceAccessManager {
public:
  ResourceAccessManager(const components::ComponentConfig& config,
                       const components::ComponentContext& context) {}
  
  bool CanAccessResource(const std::string& user_id,
                        const std::string& resource_id,
                        const std::string& resource_type,
                        const std::string& permission) {
    // Check if user is the owner
    auto ownership = GetResourceOwnership(resource_id, resource_type);
    if (!ownership.has_value()) {
      return false;
    }
    
    if (ownership->owner_id == user_id) {
      return true; // Owner has full access
    }
    
    // Check explicit permissions
    auto perm_it = ownership->permissions.find(user_id);
    if (perm_it != ownership->permissions.end()) {
      const auto& user_permissions = perm_it->second;
      if (std::find(user_permissions.begin(), user_permissions.end(), permission) != 
          user_permissions.end()) {
        return true;
      }
    }
    
    // Check shared access
    if (std::find(ownership->shared_with.begin(), ownership->shared_with.end(), user_id) != 
        ownership->shared_with.end()) {
      // Shared users have read access by default
      if (permission == "read") {
        return true;
      }
    }
    
    return false;
  }
  
  void GrantResourceAccess(const std::string& resource_id,
                          const std::string& resource_type,
                          const std::string& owner_id,
                          const std::string& user_id,
                          const std::vector<std::string>& permissions) {
    auto ownership_map = resource_ownership_.Lock();
    
    std::string key = resource_type + ":" + resource_id;
    auto& ownership = (*ownership_map)[key];
    
    ownership.resource_id = resource_id;
    ownership.resource_type = resource_type;
    ownership.owner_id = owner_id;
    
    if (!permissions.empty()) {
      ownership.permissions[user_id] = permissions;
    } else {
      // Default to shared access
      ownership.shared_with.push_back(user_id);
    }
    
    ownership.created_at = std::chrono::system_clock::now();
  }
  
  void RevokeResourceAccess(const std::string& resource_id,
                           const std::string& resource_type,
                           const std::string& user_id) {
    auto ownership_map = resource_ownership_.Lock();
    
    std::string key = resource_type + ":" + resource_id;
    auto it = ownership_map->find(key);
    if (it != ownership_map->end()) {
      auto& ownership = it->second;
      
      // Remove from permissions
      ownership.permissions.erase(user_id);
      
      // Remove from shared_with
      auto& shared = ownership.shared_with;
      shared.erase(std::remove(shared.begin(), shared.end(), user_id), shared.end());
    }
  }

private:
  std::optional<ResourceOwnership> GetResourceOwnership(const std::string& resource_id,
                                                       const std::string& resource_type) {
    auto ownership_map = resource_ownership_.Lock();
    
    std::string key = resource_type + ":" + resource_id;
    auto it = ownership_map->find(key);
    if (it != ownership_map->end()) {
      return it->second;
    }
    
    return std::nullopt;
  }
  
  concurrent::Variable<std::unordered_map<std::string, ResourceOwnership>> resource_ownership_;
};

// Resource-level authorization checker
class ResourceAuthChecker : public server::handlers::auth::AuthCheckerBase {
public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;
  
  ResourceAuthChecker(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : AuthCheckerBase(config, context),
      access_manager_(std::make_unique<ResourceAccessManager>(config, context)) {}
  
  AuthCheckResult CheckAuth(const server::http::HttpRequest& request,
                           server::request::RequestContext& context) const override {
    try {
      // Get user ID from authentication context
      auto user_id_opt = context.GetData<std::string>("user_id");
      if (!user_id_opt.has_value()) {
        return AuthCheckResult::kUserNotFound;
      }
      
      auto user_id = user_id_opt.value();
      
      // Extract resource information from request
      auto resource_info = ExtractResourceInfo(request);
      if (!resource_info.has_value()) {
        // No resource-specific authorization needed
        return AuthCheckResult::kOk;
      }
      
      // Determine required permission based on HTTP method
      std::string required_permission = GetPermissionForMethod(request.GetMethod());
      
      // Check resource access
      if (!access_manager_->CanAccessResource(user_id,
                                            resource_info->id,
                                            resource_info->type,
                                            required_permission)) {
        return AuthCheckResult::kForbidden;
      }
      
      return AuthCheckResult::kOk;
      
    } catch (const std::exception& e) {
      LOG_WARNING() << "Resource authorization failed: " << e.what();
      return AuthCheckResult::kForbidden;
    }
  }
  
  bool SupportsUserAuth() const noexcept override { return true; }

private:
  struct ResourceInfo {
    std::string id;
    std::string type;
  };
  
  std::optional<ResourceInfo> ExtractResourceInfo(const server::http::HttpRequest& request) const {
    // Extract resource information from URL path
    // Example: /api/documents/{document_id} -> resource_type: "document", resource_id: {document_id}
    
    std::string path = request.GetPath();
    
    // Simple path parsing - in real implementation, this would be more sophisticated
    if (path.find("/api/documents/") == 0) {
      size_t pos = path.find_last_of('/');
      if (pos != std::string::npos && pos < path.length() - 1) {
        ResourceInfo info;
        info.type = "document";
        info.id = path.substr(pos + 1);
        return info;
      }
    }
    
    // Add more resource types as needed
    
    return std::nullopt;
  }
  
  std::string GetPermissionForMethod(server::http::HttpMethod method) const {
    switch (method) {
      case server::http::HttpMethod::kGet: return "read";
      case server::http::HttpMethod::kPost: return "create";
      case server::http::HttpMethod::kPut: return "update";
      case server::http::HttpMethod::kPatch: return "update";
      case server::http::HttpMethod::kDelete: return "delete";
      default: return "read";
    }
  }
  
  std::unique_ptr<ResourceAccessManager> access_manager_;
};
```

## Authorization Configuration

### Static Configuration
```yaml
# Authorization component configuration
auth-rbac:
  cache-ttl: 300
  required-roles: [user, admin]
  role-hierarchy:
    admin: [user, moderator]
    moderator: [user]
    user: []
    guest: []

auth-abac:
  policy-cache-ttl: 300
  default-policies:
    - id: "public-read"
      description: "Allow public read access"
      conditions:
        resource.type: "public"
        action: "read"
      permissions: ["read"]
      active: true

auth-resource-access:
  default-permissions:
    owner: [read, write, delete, share]
    shared: [read]
    public: [read]

# Authorization factory registration
auth-checker-factories:
  - name: "rbac"
    factory: "samples::auth::RbacAuthCheckerFactory"
  - name: "abac"
    factory: "samples::auth::AbacAuthCheckerFactory"
  - name: "resource"
    factory: "samples::auth::ResourceAuthCheckerFactory"
```

### Handler Authorization Configuration
```yaml
# Handler-specific authorization configuration
handler-admin-panel:
  path: /admin/*
  task_processor: main-task-processor
  method: GET,POST,PUT,DELETE
  auth:
    types: [bearer]
    checker: rbac
    required-roles: [admin]
    scopes: [admin-access]

handler-user-profile:
  path: /api/users/{user_id}
  task_processor: main-task-processor
  method: GET,PUT
  auth:
    types: [bearer]
    checker: resource
    required-permission: read

handler-document-management:
  path: /api/documents/*
  task_processor: main-task-processor
  method: GET,POST,PUT,DELETE
  auth:
    types: [bearer]
    checker: abac
    policy-conditions:
      resource.type: "document"
      subject.roles: "user,admin"

handler-public-api:
  path: /api/public/*
  task_processor: main-task-processor
  method: GET
  auth:
    types: [bearer]
    checker: rbac
    required-roles: [guest, user, admin]
    optional: true
```

## Cross-References

### Related Framework Components
- [Authentication Patterns](authentication.md) - Authentication integration with authorization
- [Network Security Patterns](../../networking/network-security.md) - Security middleware integration
- [Component System](../../../memory-bank/main/component-system.md) - Authorization component integration
- [Framework Core](../../../memory-bank/main/framework-core.md) - Core security primitives

### Security Best Practices
- [Security Best Practices](security-best-practices.md) - Secure authorization implementation
- [Vulnerability Prevention](vulnerability-prevention.md) - Authorization-related vulnerability prevention
- [Encryption Patterns](encryption.md) - Data protection for authorization data

### Implementation References
- [`server::handlers::auth::AuthCheckerBase`](https://userver.tech/d6/db7/classserver_1_1handlers_1_1auth_1_1AuthCheckerBase.html)
- [`server::auth::UserScope`](https://userver.tech/d4/d67/classutils_1_1StrongTypedef.html)
- [`concurrent::Variable`](https://userver.tech/d8/dcc/namespaceconcurrent.html)
- [`components::PostgreCache`](https://userver.tech/d2/d8d/classcomponents_1_1PostgreCache.html)