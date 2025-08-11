# Authentication Patterns

## Overview

Comprehensive authentication patterns for userver applications, covering JWT token authentication, API key authentication, OAuth2 integration, session management, and multi-factor authentication patterns.

## Core Authentication Components

### JWT Authentication
- [`crypto::jwt::Jwt`](https://userver.tech/d7/d32/classcrypto_1_1jwt_1_1Jwt.html) - JWT token handling and validation
- [`server::handlers::auth::AuthCheckerBase`](https://userver.tech/d6/db7/classserver_1_1handlers_1_1auth_1_1AuthCheckerBase.html) - Base class for custom authentication checkers
- [`crypto::algorithm::StringsEqualConstTimeComparator`](https://userver.tech/de/d55/structcrypto_1_1algorithm_1_1StringsEqualConstTimeComparator.html) - Timing attack prevention for token comparison

### API Key Authentication
- [`components::PostgreCache`](https://userver.tech/d2/d8d/classcomponents_1_1PostgreCache.html) - Caching authentication data
- [`server::auth::UserAuthInfo::Ticket`](https://userver.tech/d4/d67/classutils_1_1StrongTypedef.html) - Strongly typed authentication tickets
- [`crypto::algorithm`](https://userver.tech/de/d55/algorithm_8hpp.html) - Cryptographic algorithms for secure comparisons

### OAuth2 Integration
- [`clients::http::Client`](https://userver.tech/d5/dee/classclients_1_1http_1_1Client.html) - HTTP client for OAuth2 provider communication
- [`formats::json`](https://userver.tech/d2/d20/md_en_2userver_2formats.html) - JSON parsing for OAuth2 responses
- [`server::http::HttpRequest`](https://userver.tech/d3/d44/classserver_1_1http_1_1HttpRequest.html) - HTTP request handling for OAuth2 flows

## JWT Authentication Implementation

### JWT Token Structure and Validation
```cpp
#include <userver/crypto/jwt/jwt.hpp>
#include <userver/crypto/algorithm.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
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

### JWT Token Generation
```cpp
#include <userver/crypto/jwt/jwt.hpp>
#include <userver/formats/json.hpp>

class JwtTokenGenerator {
public:
  JwtTokenGenerator(const std::string& secret, const std::string& algorithm = "HS256")
    : secret_(secret), algorithm_(algorithm) {}
  
  std::string GenerateToken(const std::string& user_id,
                           const std::vector<std::string>& roles,
                           int expiry_seconds = 3600) {
    auto jwt = crypto::jwt::Jwt::Create();
    
    // Set standard claims
    jwt.SetSubject(user_id);
    jwt.SetIssuedAt(std::chrono::system_clock::now());
    jwt.SetExpiration(std::chrono::system_clock::now() + 
                     std::chrono::seconds(expiry_seconds));
    
    // Set custom claims
    jwt.SetPayloadClaim("roles", formats::json::ValueBuilder(roles).ExtractValue());
    
    // Sign and return token
    return jwt.Sign(secret_, algorithm_);
  }

private:
  std::string secret_;
  std::string algorithm_;
};
```

## API Key Authentication Implementation

### API Key Authentication Checker
```cpp
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/crypto/algorithm.hpp>

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

## OAuth2 Authentication Implementation

### OAuth2 Authorization Code Flow
```cpp
#include <userver/clients/http/client.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

class OAuth2Handler : public server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-oauth2";
  
  OAuth2Handler(const components::ComponentConfig& config,
                const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      http_client_(context.FindComponent<clients::http::Client>()),
      client_id_(config["client-id"].As<std::string>()),
      client_secret_(config["client-secret"].As<std::string>()),
      redirect_uri_(config["redirect-uri"].As<std::string>()),
      auth_provider_url_(config["auth-provider-url"].As<std::string>()),
      token_endpoint_(config["token-endpoint"].As<std::string>()) {}
  
  std::string HandleRequestThrow(const server::http::HttpRequest& request,
                                server::request::RequestContext&) const override {
    if (request.GetMethod() == server::http::HttpMethod::kGet) {
      // Authorization request - redirect to OAuth2 provider
      return HandleAuthorizationRequest(request);
    } else if (request.GetMethod() == server::http::HttpMethod::kPost) {
      // Token exchange - handle callback from OAuth2 provider
      return HandleTokenExchange(request);
    }
    
    throw server::handlers::ClientError(
      server::handlers::HandlerErrorCode::kMethodNotAllowed,
      "Method not allowed"
    );
  }

private:
  std::string HandleAuthorizationRequest(const server::http::HttpRequest& request) const {
    // Generate state parameter for CSRF protection
    auto state = GenerateRandomState();
    
    // Store state in session or cache for validation
    StoreState(state, request.GetRemoteAddress());
    
    // Build authorization URL
    std::string auth_url = auth_provider_url_ + 
      "?response_type=code" +
      "&client_id=" + client_id_ +
      "&redirect_uri=" + redirect_uri_ +
      "&state=" + state +
      "&scope=read write";
    
    // Redirect to OAuth2 provider
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kFound);
    request.GetHttpResponse().SetHeader("Location", auth_url);
    
    return "";
  }
  
  std::string HandleTokenExchange(const server::http::HttpRequest& request) const {
    auto code = request.GetArg("code");
    auto state = request.GetArg("state");
    
    // Validate state parameter
    if (!ValidateState(state, request.GetRemoteAddress())) {
      throw server::handlers::ClientError(
        server::handlers::HandlerErrorCode::kBadRequest,
        "Invalid state parameter"
      );
    }
    
    // Exchange authorization code for access token
    auto token_response = ExchangeCodeForToken(code);
    
    // Store tokens in session or database
    auto user_info = GetUserInfo(token_response.access_token);
    StoreUserSession(user_info, token_response);
    
    // Redirect to application
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kFound);
    request.GetHttpResponse().SetHeader("Location", "/dashboard");
    
    return "";
  }
  
  struct TokenResponse {
    std::string access_token;
    std::string refresh_token;
    int expires_in;
    std::string token_type;
  };
  
  TokenResponse ExchangeCodeForToken(const std::string& code) const {
    formats::json::ValueBuilder request_body;
    request_body["grant_type"] = "authorization_code";
    request_body["code"] = code;
    request_body["redirect_uri"] = redirect_uri_;
    request_body["client_id"] = client_id_;
    request_body["client_secret"] = client_secret_;
    
    auto response = http_client_.CreatePost(token_endpoint_)
      .body(formats::json::ToString(request_body.ExtractValue()))
      .header("Content-Type", "application/x-www-form-urlencoded")
      .timeout(std::chrono::seconds(30))
      .perform();
    
    if (response->GetStatusCode() != 200) {
      throw std::runtime_error("Token exchange failed: " + 
                              std::to_string(response->GetStatusCode()));
    }
    
    auto json_response = formats::json::FromString(response->GetBody());
    
    TokenResponse token_response;
    token_response.access_token = json_response["access_token"].As<std::string>();
    token_response.refresh_token = json_response["refresh_token"].As<std::string>("");
    token_response.expires_in = json_response["expires_in"].As<int>();
    token_response.token_type = json_response["token_type"].As<std::string>();
    
    return token_response;
  }
  
  struct UserInfo {
    std::string user_id;
    std::string email;
    std::string name;
    std::vector<std::string> roles;
  };
  
  UserInfo GetUserInfo(const std::string& access_token) const {
    auto response = http_client_.CreateGet(auth_provider_url_ + "/userinfo")
      .header("Authorization", "Bearer " + access_token)
      .timeout(std::chrono::seconds(30))
      .perform();
    
    if (response->GetStatusCode() != 200) {
      throw std::runtime_error("Failed to get user info: " + 
                              std::to_string(response->GetStatusCode()));
    }
    
    auto json_response = formats::json::FromString(response->GetBody());
    
    UserInfo user_info;
    user_info.user_id = json_response["sub"].As<std::string>();
    user_info.email = json_response["email"].As<std::string>();
    user_info.name = json_response["name"].As<std::string>();
    // Extract roles from claims
    
    return user_info;
  }
  
  std::string GenerateRandomState() const {
    // Generate cryptographically secure random state
    return "random_state_string"; // Simplified for example
  }
  
  void StoreState(const std::string& state, const std::string& remote_addr) const {
    // Store state for validation
  }
  
  bool ValidateState(const std::string& state, const std::string& remote_addr) const {
    // Validate state parameter
    return true; // Simplified for example
  }
  
  void StoreUserSession(const UserInfo& user_info, const TokenResponse& tokens) const {
    // Store user session
  }
  
  const clients::http::Client& http_client_;
  std::string client_id_;
  std::string client_secret_;
  std::string redirect_uri_;
  std::string auth_provider_url_;
  std::string token_endpoint_;
};
```

## Session Management Implementation

### Secure Session Handler
```cpp
#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/crypto/algorithm.hpp>

class SessionManager {
public:
  struct SessionData {
    std::string user_id;
    std::vector<std::string> roles;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string remote_address;
  };
  
  SessionManager(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
    : session_timeout_(config["session-timeout"].As<int>(3600)) {}
  
  std::string CreateSession(const std::string& user_id,
                           const std::vector<std::string>& roles,
                           const std::string& remote_address) {
    auto session_id = GenerateSecureSessionId();
    
    SessionData session_data;
    session_data.user_id = user_id;
    session_data.roles = roles;
    session_data.created_at = std::chrono::system_clock::now();
    session_data.expires_at = session_data.created_at + 
                             std::chrono::seconds(session_timeout_);
    session_data.remote_address = remote_address;
    
    auto sessions = sessions_.Lock();
    (*sessions)[session_id] = std::move(session_data);
    
    return session_id;
  }
  
  std::optional<SessionData> ValidateSession(const std::string& session_id,
                                            const std::string& remote_address) {
    auto sessions = sessions_.Lock();
    auto it = sessions->find(session_id);
    
    if (it == sessions->end()) {
      return std::nullopt;
    }
    
    auto& session_data = it->second;
    
    // Check expiration
    if (std::chrono::system_clock::now() > session_data.expires_at) {
      sessions->erase(it);
      return std::nullopt;
    }
    
    // Check IP address binding (optional security feature)
    if (!session_data.remote_address.empty() && 
        session_data.remote_address != remote_address) {
      // Session hijacking attempt
      sessions->erase(it);
      return std::nullopt;
    }
    
    return session_data;
  }
  
  void DestroySession(const std::string& session_id) {
    auto sessions = sessions_.Lock();
    sessions->erase(session_id);
  }
  
  void CleanupExpiredSessions() {
    auto sessions = sessions_.Lock();
    auto now = std::chrono::system_clock::now();
    
    for (auto it = sessions->begin(); it != sessions->end();) {
      if (now > it->second.expires_at) {
        it = sessions->erase(it);
      } else {
        ++it;
      }
    }
  }

private:
  std::string GenerateSecureSessionId() const {
    // Generate cryptographically secure session ID
    return "secure_session_id"; // Simplified for example
  }
  
  concurrent::Variable<std::unordered_map<std::string, SessionData>> sessions_;
  int session_timeout_;
};
```

## Multi-Factor Authentication Implementation

### TOTP (Time-based One-Time Password) Implementation
```cpp
#include <userver/crypto/hmac.hpp>
#include <userver/formats/json.hpp>

class TotpAuthenticator {
public:
  static constexpr int TIME_STEP = 30; // 30 seconds
  static constexpr int CODE_DIGITS = 6;
  
  TotpAuthenticator(const std::string& secret)
    : secret_(secret) {}
  
  std::string GenerateSecret() {
    // Generate base32 encoded secret
    return "base32_encoded_secret"; // Simplified for example
  }
  
  std::string GenerateTotp() const {
    auto counter = GetCurrentCounter();
    return GenerateHotp(counter);
  }
  
  bool ValidateTotp(const std::string& totp_code) const {
    auto counter = GetCurrentCounter();
    
    // Check current and previous time steps for clock drift tolerance
    for (int i = -1; i <= 1; ++i) {
      auto code = GenerateHotp(counter + i);
      if (crypto::algorithm::StringsEqualConstTimeComparator{}(code, totp_code)) {
        return true;
      }
    }
    
    return false;
  }
  
  std::string GetQrCodeUrl(const std::string& user, 
                          const std::string& issuer) const {
    // Generate QR code URL for authenticator apps
    return "otpauth://totp/" + issuer + ":" + user + 
           "?secret=" + secret_ + 
           "&issuer=" + issuer +
           "&algorithm=SHA1&digits=6&period=30";
  }

private:
  uint64_t GetCurrentCounter() const {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return now / TIME_STEP;
  }
  
  std::string GenerateHotp(uint64_t counter) const {
    // Convert counter to big-endian bytes
    std::array<uint8_t, 8> counter_bytes = {
      static_cast<uint8_t>(counter >> 56),
      static_cast<uint8_t>(counter >> 48),
      static_cast<uint8_t>(counter >> 40),
      static_cast<uint8_t>(counter >> 32),
      static_cast<uint8_t>(counter >> 24),
      static_cast<uint8_t>(counter >> 16),
      static_cast<uint8_t>(counter >> 8),
      static_cast<uint8_t>(counter)
    };
    
    // Generate HMAC-SHA1
    auto hmac = crypto::hmac::HmacSha1(secret_, 
                                      std::string_view(reinterpret_cast<const char*>(counter_bytes.data()), 
                                                      counter_bytes.size()));
    
    // Dynamic truncation
    uint8_t offset = hmac[19] & 0x0F;
    uint32_t binary = ((hmac[offset] & 0x7F) << 24) |
                      ((hmac[offset + 1] & 0xFF) << 16) |
                      ((hmac[offset + 2] & 0xFF) << 8) |
                      (hmac[offset + 3] & 0xFF);
    
    // Generate code
    uint32_t code = binary % static_cast<uint32_t>(std::pow(10, CODE_DIGITS));
    
    // Pad with leading zeros
    return fmt::format("{:06d}", code);
  }
  
  std::string secret_;
};
```

## Authentication Configuration

### Static Configuration
```yaml
# Authentication component configuration
auth-jwt:
  jwt-secret: "your-jwt-secret-key"
  jwt-algorithm: "HS256"
  token-expiry: 3600

auth-api-keys:
  api-keys:
    client1-api-key:
      id: "client1"
      client-name: "Client One"
      active: true
      permissions: ["read", "write"]
      rate-limit: 1000
    client2-api-key:
      id: "client2"
      client-name: "Client Two"
      active: true
      permissions: ["read"]
      rate-limit: 500

auth-oauth2:
  client-id: "your-client-id"
  client-secret: "your-client-secret"
  redirect-uri: "https://your-app.com/oauth2/callback"
  auth-provider-url: "https://oauth2-provider.com"
  token-endpoint: "https://oauth2-provider.com/token"

auth-session:
  session-timeout: 3600
  bind-to-ip: true

auth-totp:
  time-step: 30
  code-digits: 6
```

### Handler Authentication Configuration
```yaml
# Handler-specific authentication configuration
handler-protected-api:
  path: /api/protected
  task_processor: main-task-processor
  method: GET,POST
  auth:
    types: [bearer, api-key]
    scopes: [read, write]

handler-public-api:
  path: /api/public
  task_processor: main-task-processor
  method: GET
  auth:
    types: [bearer, api-key]
    scopes: [read]
    optional: true

handler-oauth2-callback:
  path: /oauth2/callback
  task_processor: main-task-processor
  method: GET,POST
  auth:
    types: []
```

## Cross-References

### Related Framework Components
- [Network Security Patterns](../../networking/network-security.md) - TLS/SSL, rate limiting, input validation
- [HTTP Server Security](../../networking/http-https.md) - HTTPS configuration, security headers
- [Component System](../../../memory-bank/main/component-system.md) - Authentication component integration
- [Framework Core](../../../memory-bank/main/framework-core.md) - Core security primitives

### Security Best Practices
- [Security Best Practices](security-best-practices.md) - Input validation, output encoding, security headers
- [Vulnerability Prevention](vulnerability-prevention.md) - Common vulnerabilities and prevention strategies
- [Encryption Patterns](encryption.md) - TLS/SSL, data encryption, key management

### Implementation References
- [`server::handlers::auth::AuthCheckerBase`](https://userver.tech/d6/db7/classserver_1_1handlers_1_1auth_1_1AuthCheckerBase.html)
- [`crypto::jwt::Jwt`](https://userver.tech/d7/d32/classcrypto_1_1jwt_1_1Jwt.html)
- [`components::PostgreCache`](https://userver.tech/d2/d8d/classcomponents_1_1PostgreCache.html)
- [`clients::http::Client`](https://userver.tech/d5/dee/classclients_1_1http_1_1Client.html)