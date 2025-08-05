# Network Security Patterns

## Overview

Comprehensive network security patterns for userver applications, covering TLS/SSL configuration, authentication, authorization, rate limiting, input validation, and security monitoring.

## Core Security Components

### TLS/SSL Configuration
- [`engine::io::TlsWrapper`](https://userver.tech/d5/d32/classengine_1_1io_1_1TlsWrapper.html) - TLS encryption for sockets
- [`crypto::Certificate`](https://userver.tech/d6/d32/classcrypto_1_1Certificate.html) - Certificate management
- [`crypto::PrivateKey`](https://userver.tech/d7/d32/classcrypto_1_1PrivateKey.html) - Private key handling
- [`server::http::HttpRequest`](https://userver.tech/d8/d32/classserver_1_1http_1_1HttpRequest.html) - HTTPS request handling

### Authentication & Authorization
- [`server::auth::AuthChecker`](https://userver.tech/d9/d32/classserver_1_1auth_1_1AuthChecker.html) - Authentication verification
- [`server::handlers::auth::AuthCheckerBase`](https://userver.tech/da/d32/classserver_1_1handlers_1_1auth_1_1AuthCheckerBase.html) - Custom auth checkers
- [`crypto::jwt::Jwt`](https://userver.tech/db/d32/classcrypto_1_1jwt_1_1Jwt.html) - JWT token handling

### Rate Limiting & Protection
- [`server::ratelimit::RateLimiter`](https://userver.tech/dc/d32/classserver_1_1ratelimit_1_1RateLimiter.html) - Request rate limiting
- [`server::middlewares::HttpMiddlewareBase`](https://userver.tech/dd/d32/classserver_1_1middlewares_1_1HttpMiddlewareBase.html) - Security middleware

## TLS/SSL Implementation

### HTTPS Server Configuration
```cpp
#include <userver/server/component.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/crypto/certificate.hpp>

class SecureHttpServer : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "secure-http-server";
  
  SecureHttpServer(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : ComponentBase(config, context) {
    
    // Load TLS certificates
    auto cert_path = config["tls"]["cert-file"].As<std::string>();
    auto key_path = config["tls"]["key-file"].As<std::string>();
    auto ca_path = config["tls"]["ca-file"].As<std::string>("");
    
    certificate_ = crypto::Certificate::LoadFromFile(cert_path);
    private_key_ = crypto::PrivateKey::LoadFromFile(key_path);
    
    if (!ca_path.empty()) {
      ca_certificate_ = crypto::Certificate::LoadFromFile(ca_path);
    }
    
    // Configure TLS settings
    tls_config_.cipher_suites = config["tls"]["cipher-suites"].As<std::string>(
      "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
    );
    tls_config_.verify_client = config["tls"]["verify-client"].As<bool>(false);
    tls_config_.min_version = config["tls"]["min-version"].As<std::string>("TLSv1.2");
  }

private:
  struct TlsConfig {
    std::string cipher_suites;
    bool verify_client = false;
    std::string min_version = "TLSv1.2";
  };
  
  crypto::Certificate certificate_;
  crypto::PrivateKey private_key_;
  std::optional<crypto::Certificate> ca_certificate_;
  TlsConfig tls_config_;
};
```

### TLS Client Implementation
```cpp
class SecureTcpClient {
public:
  struct TlsConfig {
    std::string ca_file;
    std::string cert_file;
    std::string key_file;
    bool verify_server = true;
    std::string server_name; // For SNI
  };
  
  SecureTcpClient(const engine::io::Sockaddr& server_addr, TlsConfig tls_config)
    : server_addr_(server_addr), tls_config_(std::move(tls_config)) {}
  
  void Connect() {
    // Create TCP socket
    socket_ = engine::io::Socket::Create(
      server_addr_.Domain(),
      engine::io::SocketType::kStream
    );
    
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
    socket_.Connect(server_addr_, deadline);
    
    // Wrap with TLS
    tls_wrapper_ = engine::io::TlsWrapper::StartTlsClient(
      std::move(socket_),
      tls_config_.ca_file,
      tls_config_.cert_file,
      tls_config_.key_file
    );
    
    if (tls_config_.verify_server) {
      tls_wrapper_->SetVerifyMode(engine::io::TlsWrapper::VerifyMode::kPeer);
    }
    
    if (!tls_config_.server_name.empty()) {
      tls_wrapper_->SetServerName(tls_config_.server_name);
    }
    
    // Perform TLS handshake
    tls_wrapper_->DoHandshake(deadline);
  }
  
  std::string SendSecureRequest(const std::string& request) {
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(30));
    
    tls_wrapper_->SendAll(request.data(), request.size(), deadline);
    
    std::array<char, 4096> buffer;
    auto bytes_received = tls_wrapper_->RecvSome(buffer.data(), buffer.size(), deadline);
    
    return std::string(buffer.data(), bytes_received);
  }
  
  void Disconnect() {
    if (tls_wrapper_ && tls_wrapper_->IsValid()) {
      tls_wrapper_->Close();
    }
  }

private:
  engine::io::Sockaddr server_addr_;
  TlsConfig tls_config_;
  engine::io::Socket socket_;
  std::optional<engine::io::TlsWrapper> tls_wrapper_;
};
```

## Authentication Patterns

### JWT Authentication Handler
```cpp
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/crypto/jwt/jwt.hpp>
#include <userver/formats/json.hpp>

class JwtAuthChecker : public server::handlers::auth::AuthCheckerBase {
public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;
  
  JwtAuthChecker(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
    : AuthCheckerBase(config, context) {
    
    jwt_secret_ = config["jwt-secret"].As<std::string>();
    jwt_algorithm_ = config["jwt-algorithm"].As<std::string>("HS256");
    token_expiry_seconds_ = config["token-expiry"].As<int>(3600);
  }
  
  AuthCheckResult CheckAuth(const server::http::HttpRequest& request,
                           server::request::RequestContext& context) const override {
    try {
      // Extract JWT token from Authorization header
      auto auth_header = request.GetHeader("Authorization");
      if (auth_header.empty()) {
        return AuthCheckResult::kTokenNotFound;
      }
      
      if (!auth_header.starts_with("Bearer ")) {
        return AuthCheckResult::kTokenNotFound;
      }
      
      auto token = auth_header.substr(7); // Remove "Bearer " prefix
      
      // Verify JWT token
      auto jwt = crypto::jwt::Jwt::Parse(token);
      if (!jwt.Verify(jwt_secret_, jwt_algorithm_)) {
        return AuthCheckResult::kTokenInvalid;
      }
      
      // Check expiration
      auto exp_claim = jwt.GetPayloadClaim("exp");
      if (exp_claim.has_value()) {
        auto exp_time = std::chrono::system_clock::from_time_t(exp_claim->As<int64_t>());
        if (std::chrono::system_clock::now() > exp_time) {
          return AuthCheckResult::kTokenExpired;
        }
      }
      
      // Extract user information
      auto user_id = jwt.GetPayloadClaim("sub");
      auto roles = jwt.GetPayloadClaim("roles");
      
      if (user_id.has_value()) {
        context.SetData("user_id", user_id->As<std::string>());
      }
      
      if (roles.has_value()) {
        context.SetData("user_roles", roles->As<std::vector<std::string>>());
      }
      
      return AuthCheckResult::kOk;
      
    } catch (const std::exception& e) {
      LOG_WARNING() << "JWT authentication failed: " << e.what();
      return AuthCheckResult::kTokenInvalid;
    }
  }
  
  bool SupportsUserAuth() const noexcept override { return true; }

private:
  std::string jwt_secret_;
  std::string jwt_algorithm_;
  int token_expiry_seconds_;
};
```

### API Key Authentication
```cpp
class ApiKeyAuthChecker : public server::handlers::auth::AuthCheckerBase {
public:
  ApiKeyAuthChecker(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
    : AuthCheckerBase(config, context) {
    
    // Load API keys from configuration or database
    LoadApiKeys(config);
  }
  
  AuthCheckResult CheckAuth(const server::http::HttpRequest& request,
                           server::request::RequestContext& context) const override {
    // Try different API key locations
    std::string api_key;
    
    // 1. Check X-API-Key header
    api_key = request.GetHeader("X-API-Key");
    
    // 2. Check Authorization header with API key scheme
    if (api_key.empty()) {
      auto auth_header = request.GetHeader("Authorization");
      if (auth_header.starts_with("ApiKey ")) {
        api_key = auth_header.substr(7);
      }
    }
    
    // 3. Check query parameter
    if (api_key.empty()) {
      api_key = request.GetArg("api_key");
    }
    
    if (api_key.empty()) {
      return AuthCheckResult::kTokenNotFound;
    }
    
    // Validate API key
    auto api_keys = api_keys_.Lock();
    auto it = api_keys->find(api_key);
    
    if (it == api_keys->end()) {
      RecordFailedAuth(request.GetRemoteAddress(), "invalid_api_key");
      return AuthCheckResult::kTokenInvalid;
    }
    
    const auto& key_info = it->second;
    
    // Check if key is active
    if (!key_info.is_active) {
      return AuthCheckResult::kTokenInvalid;
    }
    
    // Check expiration
    auto now = std::chrono::system_clock::now();
    if (key_info.expires_at && now > *key_info.expires_at) {
      return AuthCheckResult::kTokenExpired;
    }
    
    // Check rate limits
    if (!CheckRateLimit(api_key, key_info.rate_limit)) {
      return AuthCheckResult::kRateLimited;
    }
    
    // Set context data
    context.SetData("api_key_id", key_info.id);
    context.SetData("client_name", key_info.client_name);
    context.SetData("permissions", key_info.permissions);
    
    RecordSuccessfulAuth(api_key);
    
    return AuthCheckResult::kOk;
  }

private:
  struct ApiKeyInfo {
    std::string id;
    std::string client_name;
    bool is_active = true;
    std::optional<std::chrono::system_clock::time_point> expires_at;
    std::vector<std::string> permissions;
    int rate_limit = 1000; // requests per hour
  };
  
  void LoadApiKeys(const components::ComponentConfig& config) {
    auto keys_config = config["api-keys"];
    auto keys = api_keys_.Lock();
    
    for (const auto& [key, info] : keys_config.Items()) {
      ApiKeyInfo key_info;
      key_info.id = info["id"].As<std::string>();
      key_info.client_name = info["client-name"].As<std::string>();
      key_info.is_active = info["active"].As<bool>(true);
      key_info.permissions = info["permissions"].As<std::vector<std::string>>();
      key_info.rate_limit = info["rate-limit"].As<int>(1000);
      
      (*keys)[key] = std::move(key_info);
    }
  }
  
  bool CheckRateLimit(const std::string& api_key, int limit) const {
    // Implement rate limiting logic
    // This is a simplified example
    return true;
  }
  
  void RecordFailedAuth(const std::string& remote_addr, const std::string& reason) const {
    LOG_WARNING() << "Failed API key authentication from " << remote_addr 
                  << ", reason: " << reason;
  }
  
  void RecordSuccessfulAuth(const std::string& api_key) const {
    // Record successful authentication for monitoring
  }
  
  concurrent::Variable<std::unordered_map<std::string, ApiKeyInfo>> api_keys_;
};
```

## Rate Limiting Implementation

### Token Bucket Rate Limiter
```cpp
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/concurrent/variable.hpp>

class TokenBucketRateLimiter : public server::middlewares::HttpMiddlewareBase {
public:
  struct Config {
    int max_tokens = 100;
    std::chrono::milliseconds refill_interval{1000};
    int refill_amount = 10;
    std::string key_extractor = "ip"; // "ip", "user", "api_key"
  };
  
  TokenBucketRateLimiter(Config config) : config_(std::move(config)) {
    // Start refill task
    refill_task_ = utils::Async("rate_limiter_refill", [this]() {
      RefillTokens();
    });
  }
  
  void HandleRequest(server::http::HttpRequest& request,
                    server::request::RequestContext& context,
                    server::middlewares::Next next) const override {
    
    auto key = ExtractKey(request, context);
    
    if (!ConsumeToken(key)) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(server::http::HttpStatus::kTooManyRequests);
      response.SetHeader("Retry-After", "60");
      response.SetData("Rate limit exceeded");
      return;
    }
    
    next(request, context);
  }

private:
  struct TokenBucket {
    int tokens;
    std::chrono::steady_clock::time_point last_refill;
    
    TokenBucket(int initial_tokens) 
      : tokens(initial_tokens), last_refill(std::chrono::steady_clock::now()) {}
  };
  
  std::string ExtractKey(const server::http::HttpRequest& request,
                        const server::request::RequestContext& context) const {
    if (config_.key_extractor == "ip") {
      return request.GetRemoteAddress();
    } else if (config_.key_extractor == "user") {
      return context.GetData<std::string>("user_id").value_or("anonymous");
    } else if (config_.key_extractor == "api_key") {
      return context.GetData<std::string>("api_key_id").value_or("unknown");
    }
    return "default";
  }
  
  bool ConsumeToken(const std::string& key) const {
    auto buckets = buckets_.Lock();
    
    auto it = buckets->find(key);
    if (it == buckets->end()) {
      // Create new bucket
      it = buckets->emplace(key, TokenBucket(config_.max_tokens)).first;
    }
    
    auto& bucket = it->second;
    
    if (bucket.tokens > 0) {
      bucket.tokens--;
      return true;
    }
    
    return false;
  }
  
  void RefillTokens() {
    while (!should_stop_) {
      engine::SleepFor(config_.refill_interval);
      
      auto buckets = buckets_.Lock();
      auto now = std::chrono::steady_clock::now();
      
      for (auto& [key, bucket] : *buckets) {
        if (now - bucket.last_refill >= config_.refill_interval) {
          bucket.tokens = std::min(bucket.tokens + config_.refill_amount, 
                                  config_.max_tokens);
          bucket.last_refill = now;
        }
      }
      
      // Clean up old buckets
      auto cleanup_threshold = now - std::chrono::hours(1);
      for (auto it = buckets->begin(); it != buckets->end();) {
        if (it->second.last_refill < cleanup_threshold) {
          it = buckets->erase(it);
        } else {
          ++it;
        }
      }
    }
  }
  
  Config config_;
  mutable concurrent::Variable<std::unordered_map<std::string, TokenBucket>> buckets_;
  engine::TaskWithResult<void> refill_task_;
  std::atomic<bool> should_stop_{false};
};
```

### Sliding Window Rate Limiter
```cpp
class SlidingWindowRateLimiter : public server::middlewares::HttpMiddlewareBase {
public:
  struct Config {
    int max_requests = 100;
    std::chrono::seconds window_size{60};
    std::string key_extractor = "ip";
  };
  
  SlidingWindowRateLimiter(Config config) : config_(std::move(config)) {}
  
  void HandleRequest(server::http::HttpRequest& request,
                    server::request::RequestContext& context,
                    server::middlewares::Next next) const override {
    
    auto key = ExtractKey(request, context);
    auto now = std::chrono::steady_clock::now();
    
    if (!IsAllowed(key, now)) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(server::http::HttpStatus::kTooManyRequests);
      response.SetHeader("X-RateLimit-Limit", std::to_string(config_.max_requests));
      response.SetHeader("X-RateLimit-Window", std::to_string(config_.window_size.count()));
      response.SetData("Rate limit exceeded");
      return;
    }
    
    RecordRequest(key, now);
    next(request, context);
  }

private:
  struct RequestWindow {
    std::deque<std::chrono::steady_clock::time_point> requests;
    mutable std::mutex mutex;
  };
  
  bool IsAllowed(const std::string& key, 
                std::chrono::steady_clock::time_point now) const {
    auto windows = windows_.Lock();
    
    auto it = windows->find(key);
    if (it == windows->end()) {
      return true; // First request for this key
    }
    
    auto& window = it->second;
    std::lock_guard<std::mutex> lock(window.mutex);
    
    // Remove old requests outside the window
    auto cutoff = now - config_.window_size;
    while (!window.requests.empty() && window.requests.front() < cutoff) {
      window.requests.pop_front();
    }
    
    return window.requests.size() < static_cast<size_t>(config_.max_requests);
  }
  
  void RecordRequest(const std::string& key,
                    std::chrono::steady_clock::time_point now) const {
    auto windows = windows_.Lock();
    
    auto& window = (*windows)[key];
    std::lock_guard<std::mutex> lock(window.mutex);
    
    window.requests.push_back(now);
  }
  
  std::string ExtractKey(const server::http::HttpRequest& request,
                        const server::request::RequestContext& context) const {
    if (config_.key_extractor == "ip") {
      return request.GetRemoteAddress();
    } else if (config_.key_extractor == "user") {
      return context.GetData<std::string>("user_id").value_or("anonymous");
    }
    return "default";
  }
  
  Config config_;
  mutable concurrent::Variable<std::unordered_map<std::string, RequestWindow>> windows_;
};
```

## Input Validation & Sanitization

### Request Validation Middleware
```cpp
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize.hpp>

class RequestValidationMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  void HandleRequest(server::http::HttpRequest& request,
                    server::request::RequestContext& context,
                    server::middlewares::Next next) const override {
    
    try {
      // Validate request size
      if (!ValidateRequestSize(request)) {
        RespondWithError(request, server::http::HttpStatus::kPayloadTooLarge,
                        "Request payload too large");
        return;
      }
      
      // Validate headers
      if (!ValidateHeaders(request)) {
        RespondWithError(request, server::http::HttpStatus::kBadRequest,
                        "Invalid headers");
        return;
      }
      
      // Validate content type for POST/PUT requests
      if (!ValidateContentType(request)) {
        RespondWithError(request, server::http::HttpStatus::kUnsupportedMediaType,
                        "Unsupported content type");
        return;
      }
      
      // Validate JSON payload if present
      if (!ValidateJsonPayload(request)) {
        RespondWithError(request, server::http::HttpStatus::kBadRequest,
                        "Invalid JSON payload");
        return;
      }
      
      // Sanitize query parameters
      SanitizeQueryParameters(request);
      
      next(request, context);
      
    } catch (const std::exception& e) {
      LOG_ERROR() << "Request validation error: " << e.what();
      RespondWithError(request, server::http::HttpStatus::kBadRequest,
                      "Request validation failed");
    }
  }

private:
  bool ValidateRequestSize(const server::http::HttpRequest& request) const {
    constexpr size_t MAX_REQUEST_SIZE = 10 * 1024 * 1024; // 10MB
    
    auto content_length = request.GetHeader("Content-Length");
    if (!content_length.empty()) {
      try {
        auto size = std::stoull(content_length);
        return size <= MAX_REQUEST_SIZE;
      } catch (const std::exception&) {
        return false;
      }
    }
    
    return request.RequestBody().size() <= MAX_REQUEST_SIZE;
  }
  
  bool ValidateHeaders(const server::http::HttpRequest& request) const {
    // Check for suspicious headers
    const std::vector<std::string> suspicious_headers = {
      "X-Forwarded-Host", "X-Original-URL", "X-Rewrite-URL"
    };
    
    for (const auto& header : suspicious_headers) {
      if (!request.GetHeader(header).empty()) {
        LOG_WARNING() << "Suspicious header detected: " << header;
        // Decide whether to block or just log
      }
    }
    
    // Validate User-Agent
    auto user_agent = request.GetHeader("User-Agent");
    if (user_agent.empty() || user_agent.size() > 512) {
      return false;
    }
    
    return true;
  }
  
  bool ValidateContentType(const server::http::HttpRequest& request) const {
    auto method = request.GetMethod();
    if (method == server::http::HttpMethod::kPost || 
        method == server::http::HttpMethod::kPut ||
        method == server::http::HttpMethod::kPatch) {
      
      auto content_type = request.GetHeader("Content-Type");
      if (content_type.empty()) {
        return false;
      }
      
      // Allow specific content types
      const std::vector<std::string> allowed_types = {
        "application/json",
        "application/x-www-form-urlencoded",
        "multipart/form-data",
        "text/plain"
      };
      
      for (const auto& allowed : allowed_types) {
        if (content_type.starts_with(allowed)) {
          return true;
        }
      }
      
      return false;
    }
    
    return true;
  }
  
  bool ValidateJsonPayload(const server::http::HttpRequest& request) const {
    auto content_type = request.GetHeader("Content-Type");
    if (!content_type.starts_with("application/json")) {
      return true; // Not JSON, skip validation
    }
    
    try {
      auto body = request.RequestBody();
      if (!body.empty()) {
        auto json = formats::json::FromString(body);
        // Additional JSON schema validation can be added here
      }
      return true;
    } catch (const std::exception& e) {
      LOG_WARNING() << "Invalid JSON payload: " << e.what();
      return false;
    }
  }
  
  void SanitizeQueryParameters(server::http::HttpRequest& request) const {
    // Remove potentially dangerous characters from query parameters
    // This is a simplified example - implement based on your needs
    
    for (const auto& [key, value] : request.GetArgMap()) {
      if (ContainsSuspiciousContent(value)) {
        LOG_WARNING() << "Suspicious query parameter detected: " << key;
        // Could remove or sanitize the parameter
      }
    }
  }
  
  bool ContainsSuspiciousContent(const std::string& value) const {
    // Check for common injection patterns
    const std::vector<std::string> suspicious_patterns = {
      "<script", "javascript:", "data:", "vbscript:",
      "onload=", "onerror=", "onclick=",
      "SELECT ", "INSERT ", "UPDATE ", "DELETE ",
      "UNION ", "DROP ", "CREATE "
    };
    
    std::string lower_value = value;
    std::transform(lower_value.begin(), lower_value.end(), 
                  lower_value.begin(), ::tolower);
    
    for (const auto& pattern : suspicious_patterns) {
      if (lower_value.find(pattern) != std::string::npos) {
        return true;
      }
    }
    
    return false;
  }
  
  void RespondWithError(server::http::HttpRequest& request,
                       server::http::HttpStatus status,
                       const std::string& message) const {
    auto& response = request.GetHttpResponse();
    response.SetStatus(status);
    response.SetHeader("Content-Type", "application/json");
    
    formats::json::ValueBuilder error_response;
    error_response["error"] = message;
    error_response["status"] = static_cast<int>(status);
    
    response.SetData(formats::json::ToString(error_response.ExtractValue()));
  }
};
```

## Security Headers Middleware

### Security Headers Implementation
```cpp
class SecurityHeadersMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  struct Config {
    bool enable_hsts = true;
    int hsts_max_age = 31536000; // 1 year
    bool enable_csp = true;
    std::string csp_policy = "default-src 'self'";
    bool enable_frame_options = true;
    std::string frame_options = "DENY";
    bool enable_content_type_options = true;
    bool enable_referrer_policy = true;
    std::string referrer_policy = "strict-origin-when-cross-origin";
  };
  
  SecurityHeadersMiddleware(Config config) : config_(std::move(config)) {}
  
  void HandleRequest(server::http::HttpRequest& request,
                    server::request::RequestContext& context,
                    server::middlewares::Next next) const override {
    
    next(request, context);
    
    // Add security headers to response
    auto& response = request.GetHttpResponse();
    
    if (config_.enable_hsts && request.IsHttps()) {
      response.SetHeader("Strict-Transport-Security", 
                        "max-age=" + std::to_string(config_.hsts_max_age) + "; includeSubDomains");
    }
    
    if (config_.enable_csp) {
      response.SetHeader("Content-Security-Policy", config_.csp_policy);
    }
    
    if (config_.enable_frame_options) {
      response.SetHeader("X-Frame-Options", config_.frame_options);
    }
    
    if (config_.enable_content_type_options) {
      response.SetHeader("X-Content-Type-Options", "nosniff");
    }
    
    if (config_.enable_referrer_policy) {
      response.SetHeader("Referrer-Policy", config_.referrer_policy);
    }
    
    // Additional security headers
    response.SetHeader("X-XSS-Protection", "1; mode=block");
    response.SetHeader("X-Permitted-Cross-Domain-Policies", "none");
    response.SetHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response.SetHeader("Pragma", "no-cache");
    response.SetHeader("Expires", "0");
  }

private:
  Config config_;
};
```

## Security Monitoring & Logging

### Security Event Logger
```cpp
#include <userver/logging/log.hpp>
#include <userver/formats/json.hpp>

class SecurityEventLogger {
public:
  enum class EventType {
    kAuthenticationFailure,
    kAuthorizationFailure,
    kRateLimitExceeded,
    kSuspiciousRequest,
    kInputValidationFailure,
    kTlsHandshakeFailure,
    kBruteForceAttempt
  };
  
  static void LogSecurityEvent(EventType event_type,
                              const server::http::HttpRequest& request,
                              const std::string& details = "") {
    formats::json::ValueBuilder event;
    
    event["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    event["event_type"] = EventTypeToString(event_type);
    event["remote_address"] = request.GetRemoteAddress();
    event["user_agent"] = request.GetHeader("User-Agent");
    event["method"] = ToString(request.GetMethod());
    event["url"] = request.GetUrl();
    event["details"] = details;
    
    if (!details.empty()) {
      event["additional_info"] = details;
    }
    
    // Add request headers for analysis
    formats::json::ValueBuilder headers;
    for (const auto& [name, value] : request.GetHeaders()) {
      headers[name] = value;
    }
    event["headers"] = headers.ExtractValue();
    
    LOG_WARNING() << "SECURITY_EVENT: " << formats::json::ToString(event.ExtractValue());
  }
  
  static void LogBruteForceAttempt(const std::string& remote_addr,
                                  const std::string& target_resource,
                                  int attempt_count) {
    formats::json::ValueBuilder event;
    event["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    event["event_type"] = "brute_force_attempt";
    event["remote_address"] = remote_addr;
    event["target_resource"] = target_resource;
    event["attempt_count"] = attempt_count;
    
    LOG_ERROR() << "SECURITY_ALERT: " << formats::json::ToString(event.ExtractValue());
  }

private:
  static std::string EventTypeToString(EventType type) {
    switch (type) {
      case EventType::kAuthenticationFailure: return "authentication_failure";
      case EventType::kAuthorizationFailure: return "authorization_failure";
      case EventType::kRateLimitExceeded: return "rate_limit_exceeded";
      case EventType::kSuspiciousRequest: return "suspicious_request";
      case EventType::kInputValidationFailure: return "input_validation_failure";
      case EventType::kTlsHandshakeFailure: return "tls_handshake_failure";
      case EventType::kBruteForceAttempt: return "brute_force_attempt";
      default: return "unknown";
    }
  }
};
```

### Intrusion Detection System
```cpp
class IntrusionDetectionSystem {
public:
  struct Config {
    int max_failed_attempts = 5;
    std::chrono::minutes lockout_duration{15};
    int suspicious_request_threshold = 10;
    std::chrono::minutes analysis_window{5};
  };
  
  IntrusionDetectionSystem(Config config) : config_(std::move(config)) {
    // Start monitoring task
    monitoring_task_ = utils::Async("ids_monitor", [this]() {
      MonitorThreats();
    });
  }
  
  bool IsBlocked(const std::string& remote_addr) const {
    auto blocked_ips = blocked_ips_.Lock();
    auto it = blocked_ips->find(remote_addr);
    
    if (it != blocked_ips->end()) {
      auto now = std::chrono::steady_clock::now();
      if (now < it->second) {
        return true; // Still blocked
      } else {
        // Block expired, remove it
        blocked_ips->erase(it);
      }
    }
    
    return false;
  }
  
  void RecordFailedAttempt(const std::string& remote_addr,
                          const std::string& resource) {
    auto attempts = failed_attempts_.Lock();
    auto& count = (*attempts)[remote_addr];
    count++;
    
    if (count >= config_.max_failed_attempts) {
      BlockIP(remote_addr, config_.lockout_duration);
      SecurityEventLogger::LogBruteForceAttempt(remote_addr, resource, count);
      
      // Reset counter after blocking
      attempts->erase(remote_addr);
    }
  }
  
  void RecordSuspiciousActivity(const std::string& remote_addr,
                               const std::string& activity_type) {
    auto activities = suspicious_activities_.Lock();
    auto& activity_list = (*activities)[remote_addr];
    
    activity_list.push_back({
      .timestamp = std::chrono::steady_clock::now(),
      .activity_type = activity_type
    });
    
    // Check if threshold exceeded
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - config_.analysis_window;
    
    // Remove old activities
    activity_list.erase(
      std::remove_if(activity_list.begin(), activity_list.end(),
        [cutoff](const SuspiciousActivity& activity) {
          return activity.timestamp < cutoff;
        }),
      activity_list.end()
    );
    
    if (activity_list.size() >= static_cast<size_t>(config_.suspicious_request_threshold)) {
      BlockIP(remote_addr, config_.lockout_duration);
      LOG_ERROR() << "IP " << remote_addr << " blocked due to suspicious activity";
    }
  }

private:
  struct SuspiciousActivity {
    std::chrono::steady_clock::time_point timestamp;
    std::string activity_type;
  };
  
  void BlockIP(const std::string& remote_addr, std::chrono::minutes duration) {
    auto blocked_ips = blocked_ips_.Lock();
    auto unblock_time = std::chrono::steady_clock::now() + duration;
    (*blocked_ips)[remote_addr] = unblock_time;
    
    LOG_WARNING() << "IP " << remote_addr << " blocked until "
                  << std::chrono::duration_cast<std::chrono::seconds>(
                       unblock_time.time_since_epoch()
                     ).count();
  }
  
  void MonitorThreats() {
    while (!should_stop_) {
      engine::SleepFor(std::chrono::minutes(1));
      
      // Clean up expired blocks
      CleanupExpiredBlocks();
      
      // Clean up old failed attempts
      CleanupOldAttempts();
      
      // Analyze patterns and generate alerts
      AnalyzePatterns();
    }
  }
  
  void CleanupExpiredBlocks() {
    auto blocked_ips = blocked_ips_.Lock();
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = blocked_ips->begin(); it != blocked_ips->end();) {
      if (now >= it->second) {
        LOG_INFO() << "Unblocking IP: " << it->first;
        it = blocked_ips->erase(it);
      } else {
        ++it;
      }
    }
  }
  
  void CleanupOldAttempts() {
    auto attempts = failed_attempts_.Lock();
    // Reset counters periodically to prevent permanent blocking
    // This is a simplified cleanup - implement more sophisticated logic as needed
    if (attempts->size() > 1000) {
      attempts->clear();
    }
  }
  
  void AnalyzePatterns() {
    // Implement pattern analysis for advanced threat detection
    // This could include:
    // - Detecting distributed attacks
    // - Identifying bot patterns
    // - Analyzing request patterns
    // - Correlating with external threat intelligence
  }
  
  Config config_;
  concurrent::Variable<std::unordered_map<std::string, std::chrono::steady_clock::time_point>> blocked_ips_;
  concurrent::Variable<std::unordered_map<std::string, int>> failed_attempts_;
  concurrent::Variable<std::unordered_map<std::string, std::vector<SuspiciousActivity>>> suspicious_activities_;
  engine::TaskWithResult<void> monitoring_task_;
  std::atomic<bool> should_stop_{false};
};
```

## CORS Security Implementation

### Secure CORS Middleware
```cpp
class SecureCorsMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  struct Config {
    std::vector<std::string> allowed_origins;
    std::vector<std::string> allowed_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
    std::vector<std::string> allowed_headers = {"Content-Type", "Authorization"};
    std::vector<std::string> exposed_headers;
    bool allow_credentials = false;
    int max_age = 86400; // 24 hours
    bool strict_origin_check = true;
  };
  
  SecureCorsMiddleware(Config config) : config_(std::move(config)) {}
  
  void HandleRequest(server::http::HttpRequest& request,
                    server::request::RequestContext& context,
                    server::middlewares::Next next) const override {
    
    auto origin = request.GetHeader("Origin");
    
    // Handle preflight requests
    if (request.GetMethod() == server::http::HttpMethod::kOptions) {
      HandlePreflightRequest(request, origin);
      return;
    }
    
    // Process actual request
    next(request, context);
    
    // Add CORS headers to response
    AddCorsHeaders(request, origin);
  }

private:
  void HandlePreflightRequest(server::http::HttpRequest& request,
                             const std::string& origin) const {
    auto& response = request.GetHttpResponse();
    
    if (!IsOriginAllowed(origin)) {
      response.SetStatus(server::http::HttpStatus::kForbidden);
      response.SetData("CORS: Origin not allowed");
      return;
    }
    
    auto requested_method = request.GetHeader("Access-Control-Request-Method");
    auto requested_headers = request.GetHeader("Access-Control-Request-Headers");
    
    if (!IsMethodAllowed(requested_method)) {
      response.SetStatus(server::http::HttpStatus::kMethodNotAllowed);
      response.SetData("CORS: Method not allowed");
      return;
    }
    
    if (!AreHeadersAllowed(requested_headers)) {
      response.SetStatus(server::http::HttpStatus::kForbidden);
      response.SetData("CORS: Headers not allowed");
      return;
    }
    
    // Set preflight response headers
    response.SetStatus(server::http::HttpStatus::kOk);
    response.SetHeader("Access-Control-Allow-Origin", origin);
    response.SetHeader("Access-Control-Allow-Methods", JoinStrings(config_.allowed_methods));
    response.SetHeader("Access-Control-Allow-Headers", JoinStrings(config_.allowed_headers));
    response.SetHeader("Access-Control-Max-Age", std::to_string(config_.max_age));
    
    if (config_.allow_credentials) {
      response.SetHeader("Access-Control-Allow-Credentials", "true");
    }
  }
  
  void AddCorsHeaders(server::http::HttpRequest& request,
                     const std::string& origin) const {
    if (!IsOriginAllowed(origin)) {
      return;
    }
    
    auto& response = request.GetHttpResponse();
    response.SetHeader("Access-Control-Allow-Origin", origin);
    
    if (!config_.exposed_headers.empty()) {
      response.SetHeader("Access-Control-Expose-Headers",
                        JoinStrings(config_.exposed_headers));
    }
    
    if (config_.allow_credentials) {
      response.SetHeader("Access-Control-Allow-Credentials", "true");
    }
  }
  
  bool IsOriginAllowed(const std::string& origin) const {
    if (origin.empty()) {
      return false;
    }
    
    if (config_.strict_origin_check) {
      return std::find(config_.allowed_origins.begin(),
                      config_.allowed_origins.end(),
                      origin) != config_.allowed_origins.end();
    }
    
    // Allow wildcard matching for less strict checking
    for (const auto& allowed : config_.allowed_origins) {
      if (allowed == "*" || origin == allowed) {
        return true;
      }
      
      // Simple wildcard matching (*.example.com)
      if (allowed.starts_with("*.")) {
        auto domain = allowed.substr(2);
        if (origin.ends_with("." + domain) || origin == domain) {
          return true;
        }
      }
    }
    
    return false;
  }
  
  bool IsMethodAllowed(const std::string& method) const {
    return std::find(config_.allowed_methods.begin(),
                    config_.allowed_methods.end(),
                    method) != config_.allowed_methods.end();
  }
  
  bool AreHeadersAllowed(const std::string& headers) const {
    if (headers.empty()) {
      return true;
    }
    
    // Parse comma-separated headers
    std::vector<std::string> requested_headers;
    std::stringstream ss(headers);
    std::string header;
    
    while (std::getline(ss, header, ',')) {
      // Trim whitespace
      header.erase(0, header.find_first_not_of(" \t"));
      header.erase(header.find_last_not_of(" \t") + 1);
      
      if (!header.empty()) {
        requested_headers.push_back(header);
      }
    }
    
    // Check if all requested headers are allowed
    for (const auto& requested : requested_headers) {
      bool found = false;
      for (const auto& allowed : config_.allowed_headers) {
        if (strcasecmp(requested.c_str(), allowed.c_str()) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
    
    return true;
  }
  
  std::string JoinStrings(const std::vector<std::string>& strings) const {
    if (strings.empty()) {
      return "";
    }
    
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
      result += ", " + strings[i];
    }
    return result;
  }
  
  Config config_;
};
```

## Security Testing Patterns

### Security Test Suite
```cpp
#include <userver/utest/utest.hpp>

class SecurityTestSuite : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup test server with security middleware
    server_config_.security_enabled = true;
    server_config_.rate_limiting_enabled = true;
    server_config_.input_validation_enabled = true;
    
    StartTestServer();
  }
  
  void TearDown() override {
    StopTestServer();
  }
  
  void StartTestServer() {
    // Implementation to start test server
  }
  
  void StopTestServer() {
    // Implementation to stop test server
  }
  
  struct ServerConfig {
    bool security_enabled = true;
    bool rate_limiting_enabled = true;
    bool input_validation_enabled = true;
  } server_config_;
};

UTEST_F(SecurityTestSuite, TestSqlInjectionPrevention) {
  // Test various SQL injection attempts
  std::vector<std::string> injection_attempts = {
    "'; DROP TABLE users; --",
    "' OR '1'='1",
    "' UNION SELECT * FROM users --",
    "admin'--",
    "' OR 1=1#"
  };
  
  for (const auto& injection : injection_attempts) {
    auto response = MakeRequest("/api/user?id=" + injection);
    
    // Should not return sensitive data or cause errors
    EXPECT_NE(response.status, server::http::HttpStatus::kInternalServerError);
    EXPECT_FALSE(response.body.find("users") != std::string::npos);
    EXPECT_FALSE(response.body.find("password") != std::string::npos);
  }
}

UTEST_F(SecurityTestSuite, TestXssProtection) {
  std::vector<std::string> xss_attempts = {
    "<script>alert('xss')</script>",
    "javascript:alert('xss')",
    "<img src=x onerror=alert('xss')>",
    "<svg onload=alert('xss')>",
    "';alert('xss');//"
  };
  
  for (const auto& xss : xss_attempts) {
    auto response = MakeRequest("/api/comment", "POST",
                               R"({"content": ")" + xss + R"("})");
    
    // Should sanitize or reject malicious content
    EXPECT_TRUE(response.status == server::http::HttpStatus::kBadRequest ||
                response.body.find("<script>") == std::string::npos);
  }
}

UTEST_F(SecurityTestSuite, TestRateLimiting) {
  constexpr int max_requests = 100;
  constexpr int excess_requests = 20;
  
  // Make requests up to the limit
  for (int i = 0; i < max_requests; ++i) {
    auto response = MakeRequest("/api/test");
    EXPECT_EQ(response.status, server::http::HttpStatus::kOk);
  }
  
  // Excess requests should be rate limited
  for (int i = 0; i < excess_requests; ++i) {
    auto response = MakeRequest("/api/test");
    EXPECT_EQ(response.status, server::http::HttpStatus::kTooManyRequests);
  }
}

UTEST_F(SecurityTestSuite, TestAuthenticationBypass) {
  // Test various authentication bypass attempts
  std::vector<std::pair<std::string, std::string>> bypass_attempts = {
    {"Authorization", "Bearer invalid_token"},
    {"Authorization", "Bearer "},
    {"Authorization", "Basic invalid"},
    {"X-API-Key", "invalid_key"},
    {"Cookie", "session=invalid"}
  };
  
  for (const auto& [header, value] : bypass_attempts) {
    auto response = MakeRequestWithHeader("/api/protected", header, value);
    EXPECT_EQ(response.status, server::http::HttpStatus::kUnauthorized);
  }
}

UTEST_F(SecurityTestSuite, TestSecurityHeaders) {
  auto response = MakeRequest("/api/test");
  
  // Check for required security headers
  EXPECT_FALSE(response.headers["Strict-Transport-Security"].empty());
  EXPECT_FALSE(response.headers["Content-Security-Policy"].empty());
  EXPECT_FALSE(response.headers["X-Frame-Options"].empty());
  EXPECT_FALSE(response.headers["X-Content-Type-Options"].empty());
  EXPECT_FALSE(response.headers["Referrer-Policy"].empty());
}

private:
  struct TestResponse {
    server::http::HttpStatus status;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
  };
  
  TestResponse MakeRequest(const std::string& path,
                          const std::string& method = "GET",
                          const std::string& body = "") {
    // Implementation to make HTTP request to test server
    return {};
  }
  
  TestResponse MakeRequestWithHeader(const std::string& path,
                                    const std::string& header_name,
                                    const std::string& header_value) {
    // Implementation to make HTTP request with specific header
    return {};
  }
};
```

## Best Practices

### Security Configuration
- Use strong TLS configurations (TLS 1.2+)
- Implement proper certificate validation
- Use secure cipher suites
- Enable HSTS for HTTPS endpoints
- Implement proper session management

### Authentication & Authorization
- Use strong authentication mechanisms (JWT, OAuth2)
- Implement proper token validation and expiration
- Use role-based access control (RBAC)
- Implement multi-factor authentication where appropriate
- Log all authentication attempts

### Input Validation
- Validate all input data (headers, parameters, body)
- Use allowlists rather than blocklists
- Implement proper encoding/escaping
- Validate content types and sizes
- Sanitize user-generated content

### Rate Limiting & DDoS Protection
- Implement multiple layers of rate limiting
- Use different limits for different endpoints
- Implement IP-based and user-based limiting
- Monitor for distributed attacks
- Use exponential backoff for failed requests

### Security Monitoring
- Log all security-relevant events
- Implement real-time alerting
- Monitor for suspicious patterns
- Use structured logging for analysis
- Implement incident response procedures

## Configuration Examples

### TLS Server Configuration
```yaml
components_manager:
  components:
    server:
      listener:
        port: 8443
        tls:
          cert-file: /etc/ssl/certs/server.crt
          key-file: /etc/ssl/private/server.key
          ca-file: /etc/ssl/certs/ca.crt
          verify-client: false
          cipher-suites: "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
          min-version: "TLSv1.2"
```

### Security Middleware Configuration
```yaml
components_manager:
  components:
    security-middleware:
      rate-limiting:
        max-requests: 100
        window-seconds: 60
        key-extractor: "ip"
      
      input-validation:
        max-request-size: 10485760  # 10MB
        allowed-content-types:
          - "application/json"
          - "application/x-www-form-urlencoded"
      
      security-headers:
        hsts:
          enabled: true
          max-age: 31536000
        csp:
          enabled: true
          policy: "default-src 'self'; script-src 'self' 'unsafe-inline'"
        frame-options: "DENY"
      
      cors:
        allowed-origins:
          - "https://example.com"
          - "https://*.example.com"
        allowed-methods: ["GET", "POST", "PUT", "DELETE"]
        allow-credentials: true
```

### Authentication Configuration
```yaml
components_manager:
  components:
    jwt-auth:
      secret: "${JWT_SECRET}"
      algorithm: "HS256"
      expiry-seconds: 3600
      
    api-key-auth:
      api-keys:
        "key123":
          id: "client1"
          client-name: "Mobile App"
          active: true
          permissions: ["read", "write"]
          rate-limit: 1000
```

## Cross-References

- **Memory Bank**: [`framework-core.md`](../../memory-bank/main/framework-core.md) - Core framework patterns
- **Memory Bank**: [`async-programming.md`](../../memory-bank/main/async-programming.md) - Asynchronous patterns
- **Rules**: [`http-https.md`](./http-https.md) - HTTP/HTTPS patterns
- **Rules**: [`grpc.md`](./grpc.md) - gRPC patterns
- **Rules**: [`websockets.md`](./websockets.md) - WebSocket patterns
- **Rules**: [`tcp-udp.md`](./tcp-udp.md) - TCP/UDP patterns
- **Rules**: [`error-handling.md`](../10-development/error-handling.md) - Error handling patterns
- **Rules**: [`testing.md`](../10-development/testing.md) - Testing strategies
- **Rules**: [`monitoring.md`](../10-development/monitoring.md) - Monitoring and observability