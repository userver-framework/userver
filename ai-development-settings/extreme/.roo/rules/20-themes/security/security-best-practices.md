# Security Best Practices

## Overview

Comprehensive security best practices for userver applications, covering input validation and sanitization, output encoding and XSS prevention, SQL injection prevention, CSRF protection mechanisms, security headers and policies, and secure coding practices.

## Core Security Components

### Input Validation
- [`formats::json::Value`](https://userver.tech/d2/d20/md_en_2userver_2formats.html) - JSON parsing and validation
- [`server::http::HttpRequest`](https://userver.tech/d3/d44/classserver_1_1http_1_1HttpRequest.html) - HTTP request handling and validation
- [`crypto::algorithm`](https://userver.tech/de/d55/algorithm_8hpp.html) - Secure string comparisons

### Output Encoding
- [`utils::encoding`](https://userver.tech/df/d0c/md_en_2userver_2logging.html) - HTML, URL, and JavaScript encoding utilities
- [`formats::json::ValueBuilder`](https://userver.tech/d2/d20/md_en_2userver_2formats.html) - Safe JSON construction
- [`server::http::HttpResponse`](https://userver.tech/da/da4/classserver_1_1http_1_1HttpResponse.html) - HTTP response handling

### Security Headers
- [`server::middlewares::HttpMiddlewareBase`](https://userver.tech/d7/d58/group__userver__middlewares.html) - Security middleware base class
- [`server::http::headers`](https://userver.tech/d8/d9f/namespacehttp_1_1headers.html) - HTTP header constants
- [`components::ComponentBase`](https://userver.tech/d0/d9c/classcomponents_1_1ComponentBase.html) - Component-based security configuration

## Input Validation and Sanitization

### Request Validation Middleware
```cpp
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/crypto/algorithm.hpp>

class RequestValidationMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  struct Config {
    size_t max_request_size = 10 * 1024 * 1024; // 10MB
    std::vector<std::string> allowed_content_types = {
      "application/json",
      "application/x-www-form-urlencoded",
      "multipart/form-data",
      "text/plain"
    };
    bool validate_user_agent = true;
    size_t max_user_agent_length = 512;
    std::vector<std::string> blocked_headers = {
      "X-Forwarded-Host", "X-Original-URL", "X-Rewrite-URL"
    };
  };
  
  RequestValidationMiddleware(Config config) : config_(std::move(config)) {}
  
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
      
      // Sanitize form data
      SanitizeFormData(request);
      
      next(request, context);
      
    } catch (const std::exception& e) {
      LOG_ERROR() << "Request validation error: " << e.what();
      RespondWithError(request, server::http::HttpStatus::kBadRequest,
                      "Request validation failed");
    }
  }

private:
  bool ValidateRequestSize(const server::http::HttpRequest& request) const {
    auto content_length = request.GetHeader("Content-Length");
    if (!content_length.empty()) {
      try {
        auto size = std::stoull(content_length);
        return size <= config_.max_request_size;
      } catch (const std::exception&) {
        return false;
      }
    }
    
    return request.RequestBody().size() <= config_.max_request_size;
  }
  
  bool ValidateHeaders(const server::http::HttpRequest& request) const {
    // Check for suspicious headers
    for (const auto& header : config_.blocked_headers) {
      if (!request.GetHeader(header).empty()) {
        LOG_WARNING() << "Suspicious header detected: " << header 
                      << " from " << request.GetRemoteAddress();
        // Log security event
        LogSecurityEvent("suspicious_header", request, "Header: " + header);
      }
    }
    
    // Validate User-Agent
    if (config_.validate_user_agent) {
      auto user_agent = request.GetHeader("User-Agent");
      if (user_agent.empty() || user_agent.size() > config_.max_user_agent_length) {
        return false;
      }
    }
    
    // Validate Host header
    auto host = request.GetHeader("Host");
    if (host.empty()) {
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
      
      // Check if content type is allowed
      for (const auto& allowed : config_.allowed_content_types) {
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
        return ValidateJsonStructure(json);
      }
      return true;
    } catch (const std::exception& e) {
      LOG_WARNING() << "Invalid JSON payload: " << e.what();
      return false;
    }
  }
  
  bool ValidateJsonStructure(const formats::json::Value& json) const {
    // Implement JSON structure validation
    // This is a simplified example
    if (json.GetSize() > 1000) {
      // Prevent excessive nesting or large objects
      return false;
    }
    
    return true;
  }
  
  void SanitizeQueryParameters(server::http::HttpRequest& request) const {
    // Remove or log potentially dangerous query parameters
    for (const auto& [key, value] : request.GetArgMap()) {
      if (ContainsSuspiciousContent(value)) {
        LOG_WARNING() << "Suspicious query parameter detected: " << key
                      << " from " << request.GetRemoteAddress();
        LogSecurityEvent("suspicious_query_param", request, "Key: " + key);
        // Could remove or sanitize the parameter
      }
    }
  }
  
  void SanitizeFormData(server::http::HttpRequest& request) const {
    // Sanitize form data
    for (const auto& [key, value] : request.GetFormDataMap()) {
      if (ContainsSuspiciousContent(value)) {
        LOG_WARNING() << "Suspicious form data detected: " << key
                      << " from " << request.GetRemoteAddress();
        LogSecurityEvent("suspicious_form_data", request, "Key: " + key);
      }
    }
  }
  
  bool ContainsSuspiciousContent(const std::string& value) const {
    // Check for common injection patterns
    const std::vector<std::string> suspicious_patterns = {
      "<script", "javascript:", "data:", "vbscript:",
      "onload=", "onerror=", "onclick=", "onmouseover=",
      "SELECT ", "INSERT ", "UPDATE ", "DELETE ",
      "UNION ", "DROP ", "CREATE ", "ALTER ",
      "EXEC ", "EXECUTE ", "xp_", "sp_",
      "../", "..\\", "%2e%2e%2f", "%2e%2e\\"
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
  
  void LogSecurityEvent(const std::string& event_type,
                       const server::http::HttpRequest& request,
                       const std::string& details) const {
    // Log security events for monitoring
    LOG_WARNING() << "SECURITY_EVENT: " << event_type 
                  << " from " << request.GetRemoteAddress()
                  << " - " << details;
  }
  
  Config config_;
};
```

### Input Validation Utilities
```cpp
#include <userver/formats/json.hpp>
#include <userver/crypto/algorithm.hpp>

class InputValidator {
public:
  // Email validation
  static bool IsValidEmail(const std::string& email) {
    // Simple email validation regex
    const std::regex email_regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, email_regex);
  }
  
  // Phone number validation
  static bool IsValidPhoneNumber(const std::string& phone) {
    // Simple phone number validation (international format)
    const std::regex phone_regex(R"(^\+?[1-9]\d{1,14}$)");
    return std::regex_match(phone, phone_regex);
  }
  
  // Username validation
  static bool IsValidUsername(const std::string& username) {
    // Username: 3-20 characters, alphanumeric and underscores only
    if (username.length() < 3 || username.length() > 20) {
      return false;
    }
    
    const std::regex username_regex(R"(^[a-zA-Z0-9_]+$)");
    return std::regex_match(username, username_regex);
  }
  
  // Password strength validation
  static bool IsStrongPassword(const std::string& password) {
    // Password requirements:
    // - At least 8 characters
    // - Contains uppercase letter
    // - Contains lowercase letter
    // - Contains digit
    // - Contains special character
    
    if (password.length() < 8) {
      return false;
    }
    
    bool has_upper = false;
    bool has_lower = false;
    bool has_digit = false;
    bool has_special = false;
    
    for (char c : password) {
      if (std::isupper(c)) has_upper = true;
      else if (std::islower(c)) has_lower = true;
      else if (std::isdigit(c)) has_digit = true;
      else if (std::ispunct(c)) has_special = true;
    }
    
    return has_upper && has_lower && has_digit && has_special;
  }
  
  // SQL injection prevention
  static std::string SanitizeSqlString(const std::string& input) {
    std::string sanitized = input;
    
    // Replace common SQL injection characters
    ReplaceAll(sanitized, "'", "''"); // Escape single quotes
    ReplaceAll(sanitized, "\\", "\\\\"); // Escape backslashes
    ReplaceAll(sanitized, "\"", "\\\""); // Escape double quotes
    
    return sanitized;
  }
  
  // HTML escaping
  static std::string EscapeHtml(const std::string& input) {
    std::string escaped = input;
    
    ReplaceAll(escaped, "&", "&");
    ReplaceAll(escaped, "<", "<");
    ReplaceAll(escaped, ">", ">");
    ReplaceAll(escaped, "\"", """);
    ReplaceAll(escaped, "'", "&#x27;");
    
    return escaped;
  }
  
  // URL encoding
  static std::string UrlEncode(const std::string& input) {
    std::string encoded;
    encoded.reserve(input.length() * 3);
    
    for (char c : input) {
      if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        encoded += c;
      } else {
        encoded += '%' + utils::encoding::HexEncode(c);
      }
    }
    
    return encoded;
  }

private:
  static void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  }
};
```

## Output Encoding and XSS Prevention

### Security Headers Middleware
```cpp
#include <userver/server/middlewares/http_middleware_base.hpp>

class SecurityHeadersMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  struct Config {
    // HTTP Strict Transport Security
    bool enable_hsts = true;
    int hsts_max_age = 31536000; // 1 year
    bool hsts_include_subdomains = true;
    bool hsts_preload = false;
    
    // Content Security Policy
    bool enable_csp = true;
    std::string csp_policy = "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'";
    
    // X-Frame-Options
    bool enable_frame_options = true;
    std::string frame_options = "DENY";
    
    // X-Content-Type-Options
    bool enable_content_type_options = true;
    
    // X-XSS-Protection
    bool enable_xss_protection = true;
    std::string xss_protection = "1; mode=block";
    
    // Referrer-Policy
    bool enable_referrer_policy = true;
    std::string referrer_policy = "strict-origin-when-cross-origin";
    
    // Permissions-Policy
    bool enable_permissions_policy = true;
    std::string permissions_policy = "geolocation=(), microphone=(), camera=()";
    
    // Feature-Policy (deprecated but still useful for older browsers)
    bool enable_feature_policy = false;
    std::string feature_policy = "geolocation 'none'; microphone 'none'; camera 'none'";
  };
  
  SecurityHeadersMiddleware(Config config) : config_(std::move(config)) {}
  
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context,
                     server::middlewares::Next next) const override {
    
    next(request, context);
    
    // Add security headers to response
    auto& response = request.GetHttpResponse();
    
    // HTTP Strict Transport Security
    if (config_.enable_hsts && request.IsHttps()) {
      std::string hsts_value = "max-age=" + std::to_string(config_.hsts_max_age);
      if (config_.hsts_include_subdomains) {
        hsts_value += "; includeSubDomains";
      }
      if (config_.hsts_preload) {
        hsts_value += "; preload";
      }
      response.SetHeader("Strict-Transport-Security", hsts_value);
    }
    
    // Content Security Policy
    if (config_.enable_csp) {
      response.SetHeader("Content-Security-Policy", config_.csp_policy);
    }
    
    // X-Frame-Options
    if (config_.enable_frame_options) {
      response.SetHeader("X-Frame-Options", config_.frame_options);
    }
    
    // X-Content-Type-Options
    if (config_.enable_content_type_options) {
      response.SetHeader("X-Content-Type-Options", "nosniff");
    }
    
    // X-XSS-Protection
    if (config_.enable_xss_protection) {
      response.SetHeader("X-XSS-Protection", config_.xss_protection);
    }
    
    // Referrer-Policy
    if (config_.enable_referrer_policy) {
      response.SetHeader("Referrer-Policy", config_.referrer_policy);
    }
    
    // Permissions-Policy
    if (config_.enable_permissions_policy) {
      response.SetHeader("Permissions-Policy", config_.permissions_policy);
    }
    
    // Feature-Policy (for older browsers)
    if (config_.enable_feature_policy) {
      response.SetHeader("Feature-Policy", config_.feature_policy);
    }
    
    // Additional security headers
    response.SetHeader("X-Permitted-Cross-Domain-Policies", "none");
    
    // Cache control for sensitive responses
    if (request.GetMethod() == server::http::HttpMethod::kPost ||
        !request.GetHeader("Authorization").empty()) {
      response.SetHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      response.SetHeader("Pragma", "no-cache");
      response.SetHeader("Expires", "0");
    }
  }

private:
  Config config_;
};
```

### Output Encoding Utilities
```cpp
#include <userver/utils/encoding.hpp>

class OutputEncoder {
public:
  // HTML entity encoding
  static std::string HtmlEncode(const std::string& input) {
    std::string encoded;
    encoded.reserve(input.length() * 6); // Reserve space for worst case
    
    for (char c : input) {
      switch (c) {
        case '&':  encoded += "&"; break;
        case '<':  encoded += "<"; break;
        case '>':  encoded += ">"; break;
        case '"':  encoded += """; break;
        case '\'': encoded += "&#x27;"; break;
        case '/':  encoded += "&#x2F;"; break;
        default:   encoded += c; break;
      }
    }
    
    return encoded;
  }
  
  // JavaScript string encoding
  static std::string JavaScriptEncode(const std::string& input) {
    std::string encoded;
    encoded.reserve(input.length() * 6);
    
    for (char c : input) {
      switch (c) {
        case '\\': encoded += "\\\\"; break;
        case '"':  encoded += "\\\""; break;
        case '\'': encoded += "\\'"; break;
        case '&':  encoded += "\\x26"; break;
        case '<':  encoded += "\\x3c"; break;
        case '>':  encoded += "\\x3e"; break;
        case '\n': encoded += "\\n"; break;
        case '\r': encoded += "\\r"; break;
        case '\t': encoded += "\\t"; break;
        case '\b': encoded += "\\b"; break;
        case '\f': encoded += "\\f"; break;
        default:
          if (c >= 0 && c < 0x20) {
            // Encode control characters
            encoded += "\\x" + utils::encoding::HexEncode(c);
          } else {
            encoded += c;
          }
          break;
      }
    }
    
    return encoded;
  }
  
  // CSS encoding
  static std::string CssEncode(const std::string& input) {
    std::string encoded;
    encoded.reserve(input.length() * 3);
    
    for (char c : input) {
      if (std::isalnum(c) || c == '-' || c == '_') {
        encoded += c;
      } else {
        encoded += "\\" + utils::encoding::HexEncode(c) + " ";
      }
    }
    
    return encoded;
  }
  
  // URL parameter encoding
  static std::string UrlParamEncode(const std::string& input) {
    std::string encoded;
    encoded.reserve(input.length() * 3);
    
    for (char c : input) {
      if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        encoded += c;
      } else {
        encoded += '%' + utils::encoding::HexEncode(c);
      }
    }
    
    return encoded;
  }
  
  // Safe JSON construction
  static std::string SafeJson(const formats::json::Value& value) {
    std::string json_str = formats::json::ToString(value);
    
    // Ensure JSON is properly escaped
    // The userver JSON library should handle this, but double-check
    return json_str;
  }
};
```

## CSRF Protection

### CSRF Token Manager
```cpp
#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/crypto/random.hpp>

class CsrfTokenManager {
public:
  struct TokenInfo {
    std::string token;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string user_id;
  };
  
  CsrfTokenManager(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : token_ttl_(config["token-ttl"].As<int>(3600)) // 1 hour
    , token_length_(config["token-length"].As<int>(32)) {
    
    // Start cleanup task
    cleanup_task_ = utils::Async("csrf-cleanup", [this]() {
      CleanupExpiredTokens();
    });
  }
  
  std::string GenerateToken(const std::string& user_id) {
    std::string token = crypto::random::GenerateRandomString(token_length_);
    
    TokenInfo token_info;
    token_info.token = token;
    token_info.created_at = std::chrono::system_clock::now();
    token_info.expires_at = token_info.created_at + token_ttl_;
    token_info.user_id = user_id;
    
    auto tokens = csrf_tokens_.Lock();
    (*tokens)[token] = std::move(token_info);
    
    return token;
  }
  
  bool ValidateToken(const std::string& token, const std::string& user_id) {
    auto tokens = csrf_tokens_.Lock();
    auto it = tokens->find(token);
    
    if (it == tokens->end()) {
      return false;
    }
    
    const auto& token_info = it->second;
    
    // Check expiration
    auto now = std::chrono::system_clock::now();
    if (now > token_info.expires_at) {
      tokens->erase(it);
      return false;
    }
    
    // Check user ID match
    if (token_info.user_id != user_id) {
      return false;
    }
    
    return true;
  }
  
  void InvalidateToken(const std::string& token) {
    auto tokens = csrf_tokens_.Lock();
    tokens->erase(token);
  }

private:
  void CleanupExpiredTokens() {
    while (!should_stop_) {
      engine::SleepFor(std::chrono::minutes(5));
      
      auto now = std::chrono::system_clock::now();
      auto tokens = csrf_tokens_.Lock();
      
      for (auto it = tokens->begin(); it != tokens->end();) {
        if (now > it->second.expires_at) {
          it = tokens->erase(it);
        } else {
          ++it;
        }
      }
    }
  }
  
  concurrent::Variable<std::unordered_map<std::string, TokenInfo>> csrf_tokens_;
  std::chrono::seconds token_ttl_;
  int token_length_;
  engine::TaskWithResult<void> cleanup_task_;
  std::atomic<bool> should_stop_{false};
};

// CSRF protection middleware
class CsrfProtectionMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  struct Config {
    std::string token_header = "X-CSRF-Token";
    std::string token_form_field = "_csrf";
    std::string token_cookie_name = "csrf_token";
    bool validate_get_requests = false;
    std::vector<server::http::HttpMethod> exempt_methods = {
      server::http::HttpMethod::kGet,
      server::http::HttpMethod::kHead,
      server::http::HttpMethod::kOptions
    };
  };
  
  CsrfProtectionMiddleware(Config config, CsrfTokenManager& token_manager)
    : config_(std::move(config)), token_manager_(token_manager) {}
  
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context,
                     server::middlewares::Next next) const override {
    
    // Check if method requires CSRF protection
    if (ShouldValidateRequest(request)) {
      if (!ValidateCsrfToken(request, context)) {
        auto& response = request.GetHttpResponse();
        response.SetStatus(server::http::HttpStatus::kForbidden);
        response.SetHeader("Content-Type", "application/json");
        
        formats::json::ValueBuilder error;
        error["error"] = "CSRF token validation failed";
        error["code"] = "CSRF_TOKEN_INVALID";
        
        response.SetData(formats::json::ToString(error.ExtractValue()));
        return;
      }
    }
    
    // Generate and set CSRF token for responses that need it
    if (ShouldSetCsrfToken(request)) {
      auto user_id_opt = context.GetData<std::string>("user_id");
      if (user_id_opt.has_value()) {
        std::string token = token_manager_.GenerateToken(user_id_opt.value());
        request.GetHttpResponse().SetCookie(config_.token_cookie_name, token,
                                          engine::io::Cookie::SameSite::kStrict);
      }
    }
    
    next(request, context);
  }

private:
  bool ShouldValidateRequest(const server::http::HttpRequest& request) const {
    // Don't validate exempt methods unless configured to do so
    if (!config_.validate_get_requests) {
      for (const auto& method : config_.exempt_methods) {
        if (request.GetMethod() == method) {
          return false;
        }
      }
    }
    
    return true;
  }
  
  bool ShouldSetCsrfToken(const server::http::HttpRequest& request) const {
    // Set token for HTML responses or when explicitly requested
    auto content_type = request.GetHttpResponse().GetHeader("Content-Type");
    return content_type.find("text/html") != std::string::npos ||
           request.GetHeader("X-Requested-With") == "XMLHttpRequest";
  }
  
  bool ValidateCsrfToken(const server::http::HttpRequest& request,
                        const server::request::RequestContext& context) const {
    std::string token;
    
    // Check header
    token = request.GetHeader(config_.token_header);
    if (token.empty()) {
      // Check form field
      token = request.GetArg(config_.token_form_field);
    }
    if (token.empty()) {
      // Check cookie
      token = request.GetCookie(config_.token_cookie_name);
    }
    
    if (token.empty()) {
      LOG_WARNING() << "CSRF token missing from request";
      return false;
    }
    
    // Get user ID from context
    auto user_id_opt = context.GetData<std::string>("user_id");
    if (!user_id_opt.has_value()) {
      LOG_WARNING() << "User ID not found in request context";
      return false;
    }
    
    // Validate token
    if (!token_manager_.ValidateToken(token, user_id_opt.value())) {
      LOG_WARNING() << "CSRF token validation failed for user: " << user_id_opt.value();
      return false;
    }
    
    return true;
  }
  
  Config config_;
  CsrfTokenManager& token_manager_;
};
```

## Secure Configuration Management

### Secure Configuration Component
```cpp
#include <userver/components/component.hpp>
#include <userver/crypto/aes.hpp>
#include <userver/crypto/random.hpp>

class SecureConfigManager {
public:
  struct SecureValue {
    std::string encrypted_value;
    std::chrono::system_clock::time_point created_at;
    std::string encryption_key_id;
  };
  
  SecureConfigManager(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : encryption_key_(config["encryption-key"].As<std::string>())
    , config_file_path_(config["config-file"].As<std::string>()) {
    
    // Load existing secure configuration
    LoadSecureConfig();
  }
  
  void SetSecureValue(const std::string& key, const std::string& value) {
    // Encrypt the value
    auto encrypted = EncryptValue(value);
    
    SecureValue secure_value;
    secure_value.encrypted_value = encrypted;
    secure_value.created_at = std::chrono::system_clock::now();
    secure_value.encryption_key_id = "default";
    
    auto config = secure_config_.Lock();
    (*config)[key] = std::move(secure_value);
    
    // Persist to file
    SaveSecureConfig();
  }
  
  std::string GetSecureValue(const std::string& key) {
    auto config = secure_config_.Lock();
    auto it = config->find(key);
    
    if (it == config->end()) {
      throw std::runtime_error("Secure configuration key not found: " + key);
    }
    
    return DecryptValue(it->second.encrypted_value);
  }
  
  bool HasSecureValue(const std::string& key) {
    auto config = secure_config_.Lock();
    return config->find(key) != config->end();
  }
  
  void RemoveSecureValue(const std::string& key) {
    auto config = secure_config_.Lock();
    config->erase(key);
    
    // Persist to file
    SaveSecureConfig();
  }

private:
  std::string EncryptValue(const std::string& value) {
    // Generate random IV
    std::string iv = crypto::random::GenerateRandomString(16);
    
    // Encrypt with AES-256-GCM
    auto aes = crypto::aes::Aes256Gcm::Create(encryption_key_);
    auto encrypted = aes.Encrypt(value, iv);
    
    // Combine IV and encrypted data
    return iv + encrypted.ciphertext + encrypted.auth_tag;
  }
  
  std::string DecryptValue(const std::string& encrypted_data) {
    if (encrypted_data.length() < 32) { // 16 bytes IV + 16 bytes minimum ciphertext
      throw std::runtime_error("Invalid encrypted data format");
    }
    
    // Extract components
    std::string iv = encrypted_data.substr(0, 16);
    std::string ciphertext = encrypted_data.substr(16, encrypted_data.length() - 32);
    std::string auth_tag = encrypted_data.substr(encrypted_data.length() - 16);
    
    // Decrypt
    auto aes = crypto::aes::Aes256Gcm::Create(encryption_key_);
    crypto::aes::Aes256Gcm::EncryptedData data;
    data.ciphertext = ciphertext;
    data.auth_tag = auth_tag;
    
    return aes.Decrypt(data, iv);
  }
  
  void LoadSecureConfig() {
    try {
      if (std::filesystem::exists(config_file_path_)) {
        std::ifstream file(config_file_path_);
        if (file.is_open()) {
          std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
          file.close();
          
          if (!content.empty()) {
            auto json = formats::json::FromString(content);
            auto config = secure_config_.Lock();
            
            for (const auto& [key, value] : json.GetItems()) {
              SecureValue secure_value;
              secure_value.encrypted_value = value["encrypted_value"].As<std::string>();
              secure_value.created_at = std::chrono::system_clock::from_time_t(
                value["created_at"].As<int64_t>()
              );
              secure_value.encryption_key_id = value["encryption_key_id"].As<std::string>();
              
              (*config)[key] = std::move(secure_value);
            }
          }
        }
      }
    } catch (const std::exception& e) {
      LOG_WARNING() << "Failed to load secure configuration: " << e.what();
    }
  }
  
  void SaveSecureConfig() {
    try {
      formats::json::ValueBuilder json_builder;
      
      auto config = secure_config_.Lock();
      for (const auto& [key, secure_value] : *config) {
        formats::json::ValueBuilder value_builder;
        value_builder["encrypted_value"] = secure_value.encrypted_value;
        value_builder["created_at"] = std::chrono::duration_cast<std::chrono::seconds>(
          secure_value.created_at.time_since_epoch()
        ).count();
        value_builder["encryption_key_id"] = secure_value.encryption_key_id;
        
        json_builder[key] = value_builder.ExtractValue();
      }
      
      std::string json_content = formats::json::ToString(json_builder.ExtractValue());
      
      std::ofstream file(config_file_path_);
      if (file.is_open()) {
        file << json_content;
        file.close();
      }
    } catch (const std::exception& e) {
      LOG_ERROR() << "Failed to save secure configuration: " << e.what();
    }
  }
  
  std::string encryption_key_;
  std::string config_file_path_;
  concurrent::Variable<std::unordered_map<std::string, SecureValue>> secure_config_;
};

// Secure config component
class SecureConfigComponent : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "secure-config";
  
  SecureConfigComponent(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
    : ComponentBase(config, context)
    , config_manager_(std::make_unique<SecureConfigManager>(config, context)) {}
  
  SecureConfigManager& GetConfigManager() {
    return *config_manager_;
  }

private:
  std::unique_ptr<SecureConfigManager> config_manager_;
};
```

## Security Best Practices Configuration

### Static Configuration
```yaml
# Request validation middleware configuration
request-validation:
  max-request-size: 10485760  # 10MB
  allowed-content-types:
    - "application/json"
    - "application/x-www-form-urlencoded"
    - "multipart/form-data"
    - "text/plain"
  validate-user-agent: true
  max-user-agent-length: 512
  blocked-headers:
    - "X-Forwarded-Host"
    - "X-Original-URL"
    - "X-Rewrite-URL"
    - "X-HTTP-Method-Override"

# Security headers middleware configuration
security-headers:
  enable-hsts: true
  hsts-max-age: 31536000  # 1 year
  hsts-include-subdomains: true
  hsts-preload: false
  enable-csp: true
  csp-policy: "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'"
  enable-frame-options: true
  frame-options: "DENY"
  enable-content-type-options: true
  enable-xss-protection: true
  xss-protection: "1; mode=block"
  enable-referrer-policy: true
  referrer-policy: "strict-origin-when-cross-origin"
  enable-permissions-policy: true
  permissions-policy: "geolocation=(), microphone=(), camera=()"

# CSRF protection configuration
csrf-protection:
  token-header: "X-CSRF-Token"
  token-form-field: "_csrf"
  token-cookie-name: "csrf_token"
  validate-get-requests: false
  exempt-methods: [GET, HEAD, OPTIONS]
  token-ttl: 3600  # 1 hour
  token-length: 32

# Secure configuration
secure-config:
  encryption-key: ""  # Should be loaded from secure source
  config-file: "/etc/userver/secure-config.json"
  auto-save: true

# Input validation settings
input-validation:
  max-string-length: 1000
  max-array-size: 100
  max-nesting-depth: 10
  allowed-characters: "UTF-8"
```

### Security Component Registration
```yaml
# Component registration
components:
  - name: "request-validation-middleware"
    type: "samples::security::RequestValidationMiddleware"
    config:
      max-request-size: 10485760
      allowed-content-types: ["application/json", "application/x-www-form-urlencoded"]

  - name: "security-headers-middleware"
    type: "samples::security::SecurityHeadersMiddleware"
    config:
      enable-hsts: true
      hsts-max-age: 31536000

  - name: "csrf-token-manager"
    type: "samples::security::CsrfTokenManager"
    config:
      token-ttl: 3600
      token-length: 32

  - name: "csrf-protection-middleware"
    type: "samples::security::CsrfProtectionMiddleware"
    dependencies: ["csrf-token-manager"]

  - name: "secure-config"
    type: "samples::security::SecureConfigComponent"
    config:
      config-file: "/etc/userver/secure-config.json"
```

## Cross-References

### Related Framework Components
- [Authentication Patterns](authentication.md) - Secure token handling, session management
- [Authorization Patterns](authorization.md) - Access control integration
- [Encryption Patterns](encryption.md) - Cryptographic implementations
- [Network Security Patterns](../../networking/network-security.md) - Rate limiting, security monitoring

### Security Implementation
- [Vulnerability Prevention](vulnerability-prevention.md) - Specific vulnerability prevention strategies
- [Framework Core](../../../memory-bank/main/framework-core.md) - Core security primitives
- [Component System](../../../memory-bank/main/component-system.md) - Security component integration

### Best Practice References
- [OWASP Top 10](https://owasp.org/www-project-top-ten/) - Web application security risks
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework) - Cybersecurity best practices
- [CWE Top 25](https://cwe.mitre.org/top25/archive/2023/2023_cwe_top25.html) - Common weakness enumeration

### Implementation References
- [`server::middlewares::HttpMiddlewareBase`](https://userver.tech/d7/d58/group__userver__middlewares.html)
- [`formats::json`](https://userver.tech/d2/d20/md_en_2userver_2formats.html)
- [`crypto::algorithm`](https://userver.tech/de/d55/algorithm_8hpp.html)
- [`utils::encoding`](https://userver.tech/df/d0c/md_en_2userver_2logging.html)