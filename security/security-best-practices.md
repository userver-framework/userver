# Security Best Practices

## Overview

Essential security best practices for userver applications, covering input validation, output encoding, secure configuration, security headers, secure coding patterns, and defense-in-depth strategies.

## Core Security Components

### Input Validation
- [`formats::json::Value`](https://userver.tech/d7/d00/classformats_1_1json_1_1Value.html) - JSON parsing and validation
- [`server::http::HttpRequest`](https://userver.tech/d3/d44/classserver_1_1http_1_1HttpRequest.html) - HTTP request validation
- [`utils::text`](https://userver.tech/d9/d00/namespaceutils_1_1text.html) - Text processing and validation utilities
- [`crypto::hash`](https://userver.tech/da/d32/namespacecrypto_1_1hash.html) - Hashing for data integrity

### Output Encoding
- [`utils::text::EscapeHtml`](https://userver.tech/d9/d00/namespaceutils_1_1text.html) - HTML entity encoding
- [`utils::text::EscapeUrl`](https://userver.tech/d9/d00/namespaceutils_1_1text.html) - URL encoding
- [`utils::text::EscapeJson`](https://userver.tech/d9/d00/namespaceutils_1_1text.html) - JSON string encoding
- [`server::http::HttpResponse`](https://userver.tech/d4/d44/classserver_1_1http_1_1HttpResponse.html) - HTTP response handling

### Security Middleware
- [`server::middlewares::HttpMiddlewareBase`](https://userver.tech/d5/d44/classserver_1_1middlewares_1_1HttpMiddlewareBase.html) - Base class for security middleware
- [`server::handlers::auth::AuthCheckerBase`](https://userver.tech/d6/d44/classserver_1_1handlers_1_1auth_1_1AuthCheckerBase.html) - Authentication checker base
- [`logging::LogExtra`](https://userver.tech/d7/d44/classlogging_1_1LogExtra.html) - Structured logging for security events

## Input Validation Patterns

### Request Validation Middleware
```cpp
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/utils/text.hpp>

class RequestValidationMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  static constexpr std::string_view kName = "request-validation";
  
  RequestValidationMiddleware(const components::ComponentConfig& config,
                             const components::ComponentContext& context)
    : HttpMiddlewareBase(config, context) {
    
    // Load validation rules
    max_request_size_ = config["max-request-size"].As<size_t>(10 * 1024 * 1024); // 10MB
    max_json_depth_ = config["max-json-depth"].As<int>(32);
    enable_sql_injection_check_ = config["enable-sql-injection-check"].As<bool>(true);
    enable_command_injection_check_ = config["enable-command-injection-check"].As<bool>(true);
    enable_xss_check_ = config["enable-xss-check"].As<bool>(true);
  }
  
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context,
                     server::middlewares::Next next) const override {
    
    try {
      // Validate request size
      ValidateRequestSize(request);
      
      // Validate HTTP headers
      ValidateHeaders(request);
      
      // Validate query parameters
      ValidateQueryParameters(request);
      
      // Validate request body based on content type
      if (request.GetMethod() == server::http::HttpMethod::kPost ||
          request.GetMethod() == server::http::HttpMethod::kPut ||
          request.GetMethod() == server::http::HttpMethod::kPatch) {
        
        auto content_type = request.GetHeader("Content-Type");
        if (content_type.find("application/json") != std::string::npos) {
          ValidateJsonBody(request);
        } else if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
          ValidateFormBody(request);
        } else if (content_type.find("multipart/form-data") != std::string::npos) {
          ValidateMultipartBody(request);
        }
      }
      
      // Call next middleware
      next(request, context);
      
    } catch (const ValidationException& e) {
      LOG_WARNING() << "Request validation failed: " << e.what();
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      request.SetResponseBody(e.what());
      return;
    } catch (const SecurityException& e) {
      LOG_WARNING() << "Security violation detected: " << e.what();
      request.SetResponseStatus(server::http::HttpStatus::kForbidden);
      request.SetResponseBody("Security violation detected");
      return;
    }
  }

private:
  class ValidationException : public std::exception {
  public:
    explicit ValidationException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
  private:
    std::string message_;
  };
  
  class SecurityException : public std::exception {
  public:
    explicit SecurityException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
  private:
    std::string message_;
  };
  
  void ValidateRequestSize(const server::http::HttpRequest& request) const {
    if (request.RequestBody().size() > max_request_size_) {
      throw ValidationException("Request body too large");
    }
  }
  
  void ValidateHeaders(const server::http::HttpRequest& request) const {
    // Validate required headers
    static const std::vector<std::string> required_headers = {
      "Host", "User-Agent", "Accept"
    };
    
    for (const auto& header : required_headers) {
      if (request.GetHeader(header).empty()) {
        throw ValidationException("Missing required header: " + header);
      }
    }
    
    // Validate header values for security issues
    for (const auto& [name, value] : request.GetHeaders()) {
      if (enable_sql_injection_check_ && ContainsSqlInjection(value)) {
        throw SecurityException("SQL injection detected in header: " + name);
      }
      
      if (enable_command_injection_check_ && ContainsCommandInjection(value)) {
        throw SecurityException("Command injection detected in header: " + name);
      }
      
      if (enable_xss_check_ && ContainsXssPatterns(value)) {
        throw SecurityException("XSS patterns detected in header: " + name);
      }
    }
  }
  
  void ValidateQueryParameters(const server::http::HttpRequest& request) const {
    for (const auto& [name, value] : request.GetQueryParameters()) {
      // Validate parameter names
      if (!IsValidParameterName(name)) {
        throw ValidationException("Invalid parameter name: " + name);
      }
      
      // Validate parameter values
      if (!IsValidParameterValue(value)) {
        throw ValidationException("Invalid parameter value for: " + name);
      }
      
      // Security checks
      if (enable_sql_injection_check_ && ContainsSqlInjection(value)) {
        throw SecurityException("SQL injection detected in parameter: " + name);
      }
      
      if (enable_command_injection_check_ && ContainsCommandInjection(value)) {
        throw SecurityException("Command injection detected in parameter: " + name);
      }
      
      if (enable_xss_check_ && ContainsXssPatterns(value)) {
        throw SecurityException("XSS patterns detected in parameter: " + name);
      }
    }
  }
  
  void ValidateJsonBody(const server::http::HttpRequest& request) const {
    try {
      auto json_value = formats::json::FromString(request.RequestBody());
      
      // Validate JSON structure depth
      ValidateJsonDepth(json_value, 0);
      
      // Validate JSON values
      ValidateJsonValue(json_value);
      
    } catch (const formats::json::ParseException& e) {
      throw ValidationException("Invalid JSON format: " + std::string(e.what()));
    }
  }
  
  void ValidateJsonDepth(const formats::json::Value& value, int current_depth) const {
    if (current_depth > max_json_depth_) {
      throw ValidationException("JSON structure too deep");
    }
    
    if (value.IsObject()) {
      for (const auto& [key, sub_value] : Items(value)) {
        ValidateJsonDepth(sub_value, current_depth + 1);
      }
    } else if (value.IsArray()) {
      for (const auto& sub_value : value) {
        ValidateJsonDepth(sub_value, current_depth + 1);
      }
    }
  }
  
  void ValidateJsonValue(const formats::json::Value& value) const {
    if (value.IsString()) {
      auto str_value = value.As<std::string>();
      
      if (enable_sql_injection_check_ && ContainsSqlInjection(str_value)) {
        throw SecurityException("SQL injection detected in JSON value");
      }
      
      if (enable_command_injection_check_ && ContainsCommandInjection(str_value)) {
        throw SecurityException("Command injection detected in JSON value");
      }
      
      if (enable_xss_check_ && ContainsXssPatterns(str_value)) {
        throw SecurityException("XSS patterns detected in JSON value");
      }
      
    } else if (value.IsObject()) {
      for (const auto& [key, sub_value] : Items(value)) {
        // Validate key
        if (!IsValidParameterName(key)) {
          throw ValidationException("Invalid JSON key: " + key);
        }
        ValidateJsonValue(sub_value);
      }
    } else if (value.IsArray()) {
      for (const auto& sub_value : value) {
        ValidateJsonValue(sub_value);
      }
    }
  }
  
  void ValidateFormBody(const server::http::HttpRequest& request) const {
    // Parse form data
    auto form_data = ParseFormData(request.RequestBody());
    
    for (const auto& [name, value] : form_data) {
      // Validate parameter names
      if (!IsValidParameterName(name)) {
        throw ValidationException("Invalid form parameter name: " + name);
      }
      
      // Validate parameter values
      if (!IsValidParameterValue(value)) {
        throw ValidationException("Invalid form parameter value for: " + name);
      }
      
      // Security checks
      if (enable_sql_injection_check_ && ContainsSqlInjection(value)) {
        throw SecurityException("SQL injection detected in form parameter: " + name);
      }
      
      if (enable_command_injection_check_ && ContainsCommandInjection(value)) {
        throw SecurityException("Command injection detected in form parameter: " + name);
      }
      
      if (enable_xss_check_ && ContainsXssPatterns(value)) {
        throw SecurityException("XSS patterns detected in form parameter: " + name);
      }
    }
  }
  
  void ValidateMultipartBody(const server::http::HttpRequest& request) const {
    // Validate multipart form data
    // This is a simplified example - in practice, you'd use a proper multipart parser
    auto content_type = request.GetHeader("Content-Type");
    if (content_type.find("boundary=") == std::string::npos) {
      throw ValidationException("Invalid multipart content type");
    }
    
    // Additional multipart validation would go here
    // Check file sizes, content types, etc.
  }
  
  bool IsValidParameterName(const std::string& name) const {
    // Allow alphanumeric characters, hyphens, and underscores
    return std::all_of(name.begin(), name.end(), [](char c) {
      return std::isalnum(c) || c == '-' || c == '_';
    }) && !name.empty() && name.size() <= 255;
  }
  
  bool IsValidParameterValue(const std::string& value) const {
    // Basic validation - check for null bytes and excessive length
    return value.find('\0') == std::string::npos && value.size() <= 65535;
  }
  
  bool ContainsSqlInjection(const std::string& input) const {
    static const std::vector<std::string> sql_keywords = {
      "SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE", "ALTER", "EXEC",
      "UNION", "SELECT", "FROM", "WHERE", "HAVING", "ORDER BY", "GROUP BY"
    };
    
    std::string upper_input = utils::text::ToUpperCase(input);
    
    for (const auto& keyword : sql_keywords) {
      if (upper_input.find(keyword) != std::string::npos) {
        // Additional checks to reduce false positives
        if (upper_input.find("'") != std::string::npos || 
            upper_input.find("--") != std::string::npos ||
            upper_input.find("/*") != std::string::npos) {
          return true;
        }
      }
    }
    
    return false;
  }
  
  bool ContainsCommandInjection(const std::string& input) const {
    static const std::vector<std::string> dangerous_chars = {
      ";", "|", "&", "`", "$(", "${"
    };
    
    static const std::vector<std::string> dangerous_commands = {
      "cat", "ls", "rm", "cp", "mv", "chmod", "chown", "wget", "curl", "ping"
    };
    
    for (const auto& chars : dangerous_chars) {
      if (input.find(chars) != std::string::npos) {
        return true;
      }
    }
    
    std::string lower_input = utils::text::ToLowerCase(input);
    for (const auto& command : dangerous_commands) {
      if (lower_input.find(command) != std::string::npos) {
        return true;
      }
    }
    
    return false;
  }
  
  bool ContainsXssPatterns(const std::string& input) const {
    static const std::vector<std::string> xss_patterns = {
      "<script", "javascript:", "onload=", "onerror=", "onclick=", "onmouseover=",
      "eval(", "document.cookie", "document.location", "window.location"
    };
    
    std::string lower_input = utils::text::ToLowerCase(input);
    for (const auto& pattern : xss_patterns) {
      if (lower_input.find(pattern) != std::string::npos) {
        return true;
      }
    }
    
    return false;
  }
  
  std::unordered_map<std::string, std::string> ParseFormData(const std::string& body) const {
    std::unordered_map<std::string, std::string> result;
    
    size_t start = 0;
    while (start < body.length()) {
      size_t end = body.find('&', start);
      if (end == std::string::npos) {
        end = body.length();
      }
      
      std::string pair = body.substr(start, end - start);
      size_t eq_pos = pair.find('=');
      
      if (eq_pos != std::string::npos) {
        std::string key = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);
        
        // URL decode
        key = utils::text::UrlDecode(key);
        value = utils::text::UrlDecode(value);
        
        result[key] = value;
      }
      
      start = end + 1;
    }
    
    return result;
  }
  
  size_t max_request_size_;
  int max_json_depth_;
  bool enable_sql_injection_check_;
  bool enable_command_injection_check_;
  bool enable_xss_check_;
};
```

## Output Encoding Patterns

### Secure Response Handler
```cpp
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/text.hpp>
#include <userver/formats/json/value.hpp>

class SecureResponseHandler : public server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "secure-response-handler";
  
  SecureResponseHandler(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    
    // Load encoding settings
    enable_html_encoding_ = config["enable-html-encoding"].As<bool>(true);
    enable_json_encoding_ = config["enable-json-encoding"].As<bool>(true);
    enable_url_encoding_ = config["enable-url-encoding"].As<bool>(true);
    content_security_policy_ = config["content-security-policy"].As<std::string>("");
  }
  
  std::string HandleRequestThrow(const server::http::HttpRequest& request,
                                server::request::RequestContext&) const override {
    
    // Generate response content
    std::string response_content = GenerateResponseContent(request);
    
    // Apply appropriate encoding based on content type
    auto content_type = request.GetHeader("Accept");
    
    if (content_type.find("text/html") != std::string::npos) {
      if (enable_html_encoding_) {
        response_content = utils::text::EscapeHtml(response_content);
      }
    } else if (content_type.find("application/json") != std::string::npos) {
      if (enable_json_encoding_) {
        // For JSON responses, ensure proper JSON encoding
        response_content = formats::json::ValueBuilder(response_content).ExtractValue().ToString();
      }
    }
    
    return response_content;
  }

private:
  std::string GenerateResponseContent(const server::http::HttpRequest& request) const {
    // Generate response content based on request
    // This is a simplified example
    
    std::string content;
    
    // Include request parameters in response (safely)
    for (const auto& [name, value] : request.GetQueryParameters()) {
      content += "<p>" + name + ": " + value + "</p>";
    }
    
    return content;
  }
  
  void SetSecurityHeaders(server::http::HttpResponse& response) const {
    // Set security headers
    if (!content_security_policy_.empty()) {
      response.SetHeader("Content-Security-Policy", content_security_policy_);
    }
    
    response.SetHeader("X-Content-Type-Options", "nosniff");
    response.SetHeader("X-Frame-Options", "DENY");
    response.SetHeader("X-XSS-Protection", "1; mode=block");
    response.SetHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
    response.SetHeader("Referrer-Policy", "strict-origin-when-cross-origin");
  }
  
  bool enable_html_encoding_;
  bool enable_json_encoding_;
  bool enable_url_encoding_;
  std::string content_security_policy_;
};
```

## CSRF Protection Implementation

### CSRF Token Manager
```cpp
#include <userver/components/component.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/crypto/random.hpp>
#include <userver/crypto/hash.hpp>

class CsrfTokenManager {
public:
  struct CsrfToken {
    std::string token;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string session_id;
  };
  
  CsrfTokenManager(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
    : token_ttl_(config["token-ttl"].As<int>(3600)) // 1 hour
    , token_length_(config["token-length"].As<int>(32)) {
    
    // Start token cleanup task
    cleanup_task_ = utils::Async("csrf-token-cleanup", [this]() {
      TokenCleanupLoop();
    });
  }
  
  std::string GenerateToken(const std::string& session_id) {
    std::string token = crypto::random::GenerateRandomString(token_length_);
    std::string hashed_token = crypto::hash::Sha256::Hash(token);
    
    CsrfToken csrf_token;
    csrf_token.token = hashed_token;
    csrf_token.created_at = std::chrono::system_clock::now();
    csrf_token.expires_at = csrf_token.created_at + token_ttl_;
    csrf_token.session_id = session_id;
    
    auto tokens = token_store_.Lock();
    (*tokens)[hashed_token] = csrf_token;
    
    return token;
  }
  
  bool ValidateToken(const std::string& token, const std::string& session_id) {
    std::string hashed_token = crypto::hash::Sha256::Hash(token);
    
    auto tokens = token_store_.Lock();
    auto it = tokens->find(hashed_token);
    
    if (it == tokens->end()) {
      return false;
    }
    
    const auto& csrf_token = it->second;
    
    // Check expiration
    if (std::chrono::system_clock::now() > csrf_token.expires_at) {
      tokens->erase(it);
      return false;
    }
    
    // Check session match
    if (csrf_token.session_id != session_id) {
      return false;
    }
    
    return true;
  }
  
  void InvalidateToken(const std::string& token) {
    std::string hashed_token = crypto::hash::Sha256::Hash(token);
    
    auto tokens = token_store_.Lock();
    tokens->erase(hashed_token);
  }

private:
  void TokenCleanupLoop() {
    while (!should_stop_) {
      engine::SleepFor(std::chrono::minutes(10));
      
      try {
        CleanupExpiredTokens();
      } catch (const std::exception& e) {
        LOG_ERROR() << "CSRF token cleanup failed: " << e.what();
      }
    }
  }
  
  void CleanupExpiredTokens() {
    auto now = std::chrono::system_clock::now();
    
    auto tokens = token_store_.Lock();
    for (auto it = tokens->begin(); it != tokens->end();) {
      if (now > it->second.expires_at) {
        it = tokens->erase(it);
      } else {
        ++it;
      }
    }
  }
  
  concurrent::Variable<std::unordered_map<std::string, CsrfToken>> token_store_;
  std::chrono::seconds token_ttl_;
  int token_length_;
  engine::TaskWithResult<void> cleanup_task_;
  std::atomic<bool> should_stop_{false};
};

// CSRF protection middleware
class CsrfProtectionMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  static constexpr std::string_view kName = "csrf-protection";
  
  CsrfProtectionMiddleware(const components::ComponentConfig& config,
                          const components::ComponentContext& context)
    : HttpMiddlewareBase(config, context)
    , csrf_manager_(context.FindComponent<CsrfTokenManager>()) {
    
    exempt_methods_ = config["exempt-methods"].As<std::vector<std::string>>({"GET", "HEAD", "OPTIONS"});
    exempt_paths_ = config["exempt-paths"].As<std::vector<std::string>>({"/api/health", "/metrics"});
    token_header_ = config["token-header"].As<std::string>("X-CSRF-Token");
    token_form_field_ = config["token-form-field"].As<std::string>("_csrf_token");
  }
  
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context,
                     server::middlewares::Next next) const override {
    
    std::string path = request.GetUrl();
    std::string method = ToString(request.GetMethod());
    
    // Check if path/method is exempt
    if (IsExemptPath(path) || IsExemptMethod(method)) {
      next(request, context);
      return;
    }
    
    // For state-changing methods, validate CSRF token
    if (IsStateChangingMethod(method)) {
      if (!ValidateCsrfToken(request)) {
        LOG_WARNING() << "CSRF token validation failed for " << method << " " << path;
        request.SetResponseStatus(server::http::HttpStatus::kForbidden);
        request.SetResponseBody("CSRF token validation failed");
        return;
      }
    }
    
    // Generate CSRF token for all requests (for use in forms)
    std::string session_id = GetSessionId(request);
    std::string csrf_token = csrf_manager_.GenerateToken(session_id);
    request.GetResponse().SetHeader("X-CSRF-Token", csrf_token);
    
    next(request, context);
  }

private:
  bool IsExemptPath(const std::string& path) const {
    for (const auto& exempt_path : exempt_paths_) {
      if (path == exempt_path || (exempt_path.back() == '*' && 
          path.substr(0, exempt_path.length() - 1) == exempt_path.substr(0, exempt_path.length() - 1))) {
        return true;
      }
    }
    return false;
  }
  
  bool IsExemptMethod(const std::string& method) const {
    return std::find(exempt_methods_.begin(), exempt_methods_.end(), method) != exempt_methods_.end();
  }
  
  bool IsStateChangingMethod(const std::string& method) const {
    static const std::vector<std::string> state_changing_methods = {"POST", "PUT", "PATCH", "DELETE"};
    return std::find(state_changing_methods.begin(), state_changing_methods.end(), method) != state_changing_methods.end();
  }
  
  bool ValidateCsrfToken(const server::http::HttpRequest& request) const {
    std::string token;
    
    // Check header first
    token = request.GetHeader(token_header_);
    
    // Check form data if header not found
    if (token.empty() && request.GetMethod() == server::http::HttpMethod::kPost) {
      auto content_type = request.GetHeader("Content-Type");
      if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
        // Parse form data to find token
        auto form_data = ParseFormData(request.RequestBody());
        auto it = form_data.find(token_form_field_);
        if (it != form_data.end()) {
          token = it->second;
        }
      }
    }
    
    if (token.empty()) {
      return false;
    }
    
    std::string session_id = GetSessionId(request);
    return csrf_manager_.ValidateToken(token, session_id);
  }
  
  std::string GetSessionId(const server::http::HttpRequest& request) const {
    // Extract session ID from request (this is a simplified example)
    // In practice, you'd get this from your session management system
    return request.GetHeader("X-Session-ID");
  }
  
  std::unordered_map<std::string, std::string> ParseFormData(const std::string& body) const {
    std::unordered_map<std::string, std::string> result;
    
    size_t start = 0;
    while (start < body.length()) {
      size_t end = body.find('&', start);
      if (end == std::string::npos) {
        end = body.length();
      }
      
      std::string pair = body.substr(start, end - start);
      size_t eq_pos = pair.find('=');
      
      if (eq_pos != std::string::npos) {
        std::string key = pair.substr(0, eq_pos);
        std::string value = pair.substr(eq_pos + 1);
        
        // URL decode
        key = utils::text::UrlDecode(key);
        value = utils::text::UrlDecode(value);
        
        result[key] = value;
      }
      
      start = end + 1;
    }
    
    return result;
  }
  
  CsrfTokenManager& csrf_manager_;
  std::vector<std::string> exempt_methods_;
  std::vector<std::string> exempt_paths_;
  std::string token_header_;
  std::string token_form_field_;
};
```

## Security Headers Implementation

### Security Headers Middleware
```cpp
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/server/http/http_response.hpp>

class SecurityHeadersMiddleware : public server::middlewares::HttpMiddlewareBase {
public:
  static constexpr std::string_view kName = "security-headers";
  
  SecurityHeadersMiddleware(const components::ComponentConfig& config,
                           const components::ComponentContext& context)
    : HttpMiddlewareBase(config, context) {
    
    // Load security header configurations
    csp_enabled_ = config["content-security-policy"]["enabled"].As<bool>(true);
    csp_policy_ = config["content-security-policy"]["policy"].As<std::string>(
      "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'"
    );
    
    hsts_enabled_ = config["strict-transport-security"]["enabled"].As<bool>(true);
    hsts_max_age_ = config["strict-transport-security"]["max-age"].As<int>(31536000); // 1 year
    hsts_include_subdomains_ = config["strict-transport-security"]["include-subdomains"].As<bool>(true);
    hsts_preload_ = config["strict-transport-security"]["preload"].As<bool>(false);
    
    x_content_type_options_ = config["x-content-type-options"].As<std::string>("nosniff");
    x_frame_options_ = config["x-frame-options"].As<std::string>("DENY");
    x_xss_protection_ = config["x-xss-protection"].As<std::string>("1; mode=block");
    referrer_policy_ = config["referrer-policy"].As<std::string>("strict-origin-when-cross-origin");
    permissions_policy_ = config["permissions-policy"].As<std::string>("");
  }
  
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context,
                     server::middlewares::Next next) const override {
    
    // Call next middleware first to let the handler process the request
    next(request, context);
    
    // Set security headers on the response
    auto& response = request.GetResponse();
    SetSecurityHeaders(response, request);
  }

private:
  void SetSecurityHeaders(server::http::HttpResponse& response,
                         const server::http::HttpRequest& request) const {
    
    // Content Security Policy
    if (csp_enabled_) {
      response.SetHeader("Content-Security-Policy", csp_policy_);
    }
    
    // Strict Transport Security
    if (hsts_enabled_ && request.IsSecure()) {
      std::string hsts_value = "max-age=" + std::to_string(hsts_max_age_);
      if (hsts_include_subdomains_) {
        hsts_value += "; includeSubDomains";
      }
      if (hsts_preload_) {
        hsts_value += "; preload";
      }
      response.SetHeader("Strict-Transport-Security", hsts_value);
    }
    
    // X-Content-Type-Options
    if (!x_content_type_options_.empty()) {
      response.SetHeader("X-Content-Type-Options", x_content_type_options_);
    }
    
    // X-Frame-Options
    if (!x_frame_options_.empty()) {
      response.SetHeader("X-Frame-Options", x_frame_options_);
    }
    
    // X-XSS-Protection
    if (!x_xss_protection_.empty()) {
      response.SetHeader("X-XSS-Protection", x_xss_protection_);
    }
    
    // Referrer-Policy
    if (!referrer_policy_.empty()) {
      response.SetHeader("Referrer-Policy", referrer_policy_);
    }
    
    // Permissions-Policy
    if (!permissions_policy_.empty()) {
      response.SetHeader("Permissions-Policy", permissions_policy_);
    }
  }
  
  bool csp_enabled_;
  std::string csp_policy_;
  bool hsts_enabled_;
  int hsts_max_age_;
  bool hsts_include_subdomains_;
  bool hsts_preload_;
  std::string x_content_type_options_;
  std::string x_frame_options_;
  std::string x_xss_protection_;
  std::string referrer_policy_;
  std::string permissions_policy_;
};
```

## Secure Configuration Patterns

### Configuration Validator
```cpp
#include <userver/components/component.hpp>
#include <userver/formats/json/value.hpp>

class SecureConfigValidator {
public:
  struct ValidationResult {
    bool is_valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
  };
  
  static ValidationResult ValidateConfiguration(const components::ComponentConfig& config) {
    ValidationResult result;
    
    // Validate security-related configuration
    ValidateTlsConfiguration(config, result);
    ValidateAuthenticationConfiguration(config, result);
    ValidateRateLimitingConfiguration(config, result);
    ValidateSecurityHeadersConfiguration(config, result);
    
    return result;
  }

private:
  static void ValidateTlsConfiguration(const components::ComponentConfig& config,
                                      ValidationResult& result) {
    if (config.HasMember("tls")) {
      const auto& tls_config = config["tls"];
      
      // Check for required TLS settings
      if (!tls_config.HasMember("cert-file") || !tls_config.HasMember("key-file")) {
        result.errors.push_back("TLS configuration missing cert-file or key-file");
        result.is_valid = false;
      }
      
      // Check TLS version
      if (tls_config.HasMember("min-version")) {
        auto min_version = tls_config["min-version"].As<std::string>();
        if (min_version < "TLSv1.2") {
          result.warnings.push_back("TLS minimum version should be TLSv1.2 or higher");
        }
      }
      
      // Check cipher suites
      if (tls_config.HasMember("cipher-suites")) {
        auto cipher_suites = tls_config["cipher-suites"].As<std::string>();
        if (cipher_suites.find("NULL") != std::string::npos ||
            cipher_suites.find("EXPORT") != std::string::npos) {
          result.errors.push_back("Weak cipher suites detected in TLS configuration");
          result.is_valid = false;
        }
      }
    }
  }
  
  static void ValidateAuthenticationConfiguration(const components::ComponentConfig& config,
                                                ValidationResult& result) {
    if (config.HasMember("auth")) {
      const auto& auth_config = config["auth"];
      
      // Check JWT configuration
      if (auth_config.HasMember("jwt")) {
        const auto& jwt_config = auth_config["jwt"];
        if (!jwt_config.HasMember("secret") && !jwt_config.HasMember("public-key-file")) {
          result.warnings.push_back("JWT configuration should specify either secret or public-key-file");
        }
      }
      
      // Check session configuration
      if (auth_config.HasMember("session")) {
        const auto& session_config = auth_config["session"];
        if (session_config.HasMember("secure") && !session_config["secure"].As<bool>()) {
          result.warnings.push_back("Session cookies should be secure in production");
        }
      }
    }
  }
  
  static void ValidateRateLimitingConfiguration(const components::ComponentConfig& config,
                                               ValidationResult& result) {
    if (config.HasMember("rate-limiting")) {
      const auto& rate_config = config["rate-limiting"];
      
      // Check if rate limiting is configured
      if (!rate_config.HasMember("enabled") || !rate_config["enabled"].As<bool>()) {
        result.warnings.push_back("Rate limiting is disabled - consider enabling for security");
      }
    } else {
      result.warnings.push_back("Rate limiting configuration not found - consider adding");
    }
  }
  
  static void ValidateSecurityHeadersConfiguration(const components::ComponentConfig& config,
                                                  ValidationResult& result) {
    if (config.HasMember("security-headers")) {
      const auto& headers_config = config["security-headers"];
      
      // Check for essential security headers
      if (!headers_config.HasMember("content-security-policy")) {
        result.warnings.push_back("Content-Security-Policy not configured");
      }
      
      if (!headers_config.HasMember("strict-transport-security")) {
        result.warnings.push_back("Strict-Transport-Security not configured");
      }
    }
  }
};

// Configuration validation component
class ConfigValidationComponent : public components::ComponentBase {
public:
  static constexpr std::string_view kName = "config-validation";
  
  ConfigValidationComponent(const components::ComponentConfig& config,
                           const components::ComponentContext& context)
    : ComponentBase(config, context) {
    
    // Validate configuration on startup
    auto validation_result = SecureConfigValidator::ValidateConfiguration(config);
    
    if (!validation_result.is_valid) {
      std::string error_msg = "Configuration validation failed:\n";
      for (const auto& error : validation_result.errors) {
        error_msg += "  - " + error + "\n";
      }
      throw std::runtime_error(error_msg);
    }
    
    if (!validation_result.warnings.empty()) {
      LOG_WARNING() << "Configuration validation warnings:";
      for (const auto& warning : validation_result.warnings) {
        LOG_WARNING() << "  - " << warning;
      }
    }
    
    LOG_INFO() << "Configuration validation completed successfully";
  }
};
```

## Security Best Practices Configuration

### Static Configuration
```yaml
# Request validation middleware configuration
request-validation:
  max-request-size: 10485760  # 10MB
  max-json-depth: 32
  enable-sql-injection-check: true
  enable-command-injection-check: true
  enable-xss-check: true

# CSRF protection configuration
csrf-protection:
  token-ttl: 3600  # 1 hour
  token-length: 32
  exempt-methods: ["GET", "HEAD", "OPTIONS"]
  exempt-paths: ["/api/health", "/metrics", "/static/*"]
  token-header: "X-CSRF-Token"
  token-form-field: "_csrf_token"

# Security headers configuration
security-headers:
  content-security-policy:
    enabled: true
    policy: "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; font-src 'self'; connect-src 'self'; frame-ancestors 'none';"
  
  strict-transport-security:
    enabled: true
    max-age: 31536000  # 1 year
    include-subdomains: true
    preload: false
  
  x-content-type-options: "nosniff"
  x-frame-options: "DENY"
  x-xss-protection: "1; mode=block"
  referrer-policy: "strict-origin-when-cross-origin"
  permissions-policy: "geolocation=(), microphone=(), camera=()"

# Secure configuration validation
config-validation:
  validate-on-startup: true
  fail-on-validation-error: true

# Rate limiting configuration
rate-limiting:
  enabled: true
  default-limit: 1000
  default-window: 3600  # 1 hour
  ip-based-limiting: true
  user-based-limiting: true
```

## Cross-References

### Related Framework Components
- [Authentication Patterns](authentication.md) - Secure token handling, session management
- [Authorization Patterns](authorization.md) - Access control validation
- [Encryption Patterns](encryption.md) - Data protection, secure communication
- [Component System](../../memory-bank/main/component-system.md) - Security middleware integration
- [Framework Core](../../memory-bank/main/framework-core.md) - Core security utilities

### Security Implementation References
- [Vulnerability Prevention](vulnerability-prevention.md) - Specific vulnerability mitigation strategies
- [Network Security](../networking/network-security.md) - Network-level security measures
- [Database Security](../database/database-security.md) - Database security best practices

### Implementation References
- [`server::middlewares::HttpMiddlewareBase`](https://userver.tech/d5/d44/classserver_1_1middlewares_1_1HttpMiddlewareBase.html)
- [`formats::json::Value`](https://userver.tech/d7/d00/classformats_1_1json_1_1Value.html)
- [`utils::text`](https://userver.tech/d9/d00/namespaceutils_1_1text.html)
- [`crypto::hash`](https://userver.tech/da/d32/namespacecrypto_1_1hash.html)
- [`logging::LogExtra`](https://userver.tech/d7/d44/classlogging_1_1LogExtra.html)