# HTTP/HTTPS Implementation Patterns

## Overview

HTTP/HTTPS implementation patterns for userver applications, covering server and client patterns, streaming APIs, middleware development, and performance optimization.

## Quality Tiers
- **HTTP 1.x**: Platinum Tier
- **HTTPS 1.x**: Golden Tier  
- **HTTP 2.0**: Silver Tier
- **WebSocket**: Golden Tier

## HTTP Server Patterns

### Server Configuration
```yaml
components_manager:
  components:
    server:
      listener:
        port: 8080
        task_processor: main-task-processor
      connection:
        http-version: '2'  # enum `1.1` or `2`
        http2-session:
          max_concurrent_streams: 100
          max_frame_size: 16384
          initial_window_size: 65536
```

### Handler Implementation
```cpp
#include <userver/server/handlers/http_handler_base.hpp>

class MyHandler final : public server::handlers::HttpHandlerBase {
public:
  std::string HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& context
  ) const override {
    // Handle request logic
    return response_data;
  }
};
```

### Streaming API Implementation
```cpp
#include <userver/server/http/http_response_body_stream_fwd.hpp>

void HandleStreamRequest(
  server::http::HttpRequest& request,
  server::request::RequestContext& context,
  server::http::ResponseBodyStream& response_body_stream
) const override {
  // Set headers first
  response_body_stream.SetHeader("Content-Type", "application/json");
  response_body_stream.SetStatusCode(server::http::HttpStatus::kOk);
  response_body_stream.SetEndOfHeaders();
  
  // Stream data chunks
  while (has_more_data) {
    std::string chunk = get_next_chunk();
    response_body_stream.PushBodyChunk(
      std::move(chunk), 
      engine::Deadline()
    );
  }
}
```

### Enable Streaming in Config
```yaml
components_manager:
  components:
    handler-stream-api:
      response-body-stream: true
```

## HTTP Client Patterns

### Client Creation and Usage
```cpp
#include <userver/clients/http/client.hpp>

// Get client from component
auto& http_client = context.FindComponent<components::HttpClient>().GetHttpClient();

// Create and execute request
const auto response = http_client
  .CreateRequest()
  .post(url, data)
  .timeout(std::chrono::seconds(1))
  .perform();

EXPECT_TRUE(response->IsOk());
```

### Advanced Client Configuration
```cpp
// Custom headers and authentication
auto request = http_client.CreateRequest()
  .get(url)
  .headers({{"Authorization", "Bearer " + token}})
  .timeout(std::chrono::seconds(5))
  .retry(3);

auto response = request.perform();
```

### Streaming Client Requests
```cpp
auto queue = concurrent::StringStreamQueue::Create();
auto client_response = external_request.async_perform_stream_body(std::move(queue));

// Process headers
for (const auto& header_item : client_response.GetHeaders()) {
  response_body_stream.SetHeader(header_item.first, header_item.second);
}

// Stream response body
std::string body_part;
auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
while (client_response.ReadChunk(body_part, deadline)) {
  response_body_stream.PushBodyChunk(std::move(body_part), engine::Deadline());
}
```

## HTTPS/TLS Configuration

### Server TLS Setup
```yaml
components_manager:
  components:
    server:
      listener:
        port: 8443
        tls:
          cert-file: /path/to/server.crt
          private-key-file: /path/to/server.key
          ca-file: /path/to/ca.crt  # Optional for client cert verification
```

### Client TLS Setup
```yaml
components_manager:
  components:
    http-client:
      tls:
        ca-file: /path/to/ca.crt
        cert-file: /path/to/client.crt
        private-key-file: /path/to/client.key
```

## Middleware Development

### Custom Middleware Implementation
```cpp
#include <userver/server/middlewares/http_middleware_base.hpp>

class CustomMiddleware final : public server::middlewares::HttpMiddlewareBase {
public:
  void HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext& context,
    server::middlewares::HttpMiddlewareCallContext& call_context
  ) const override {
    // Pre-processing logic
    LOG_INFO() << "Processing request: " << request.GetUrl();
    
    // Call next middleware/handler
    call_context.Next(request, context);
    
    // Post-processing logic
    LOG_INFO() << "Request completed";
  }
};
```

### Middleware Registration
```cpp
// In component constructor
RegisterMiddleware(std::make_unique<CustomMiddleware>());
```

## Performance Optimization

### Connection Pooling
```yaml
http-client:
  pool-statistics-disable: false
  thread-name-prefix: http-client
  destination-metrics-auto-max-size: 100
  user-agent: userver-http-client/1.0
  testsuite-enabled: true
  testsuite-timeout: 5s
  testsuite-allowed-url-prefixes: ['http://localhost/']
```

### Request Limits and Timeouts
```yaml
server:
  max_request_size: 1024
  max_url_size: 8192
  max_headers_size: 65536
  request_timeout: 10s
  response_timeout: 15s
```

### HTTP/2 Optimization
```yaml
server:
  connection:
    http-version: '2'
    http2-session:
      max_concurrent_streams: 1000
      max_frame_size: 32768
      initial_window_size: 131072
      max_header_list_size: 16384
```

## Error Handling

### Client Error Handling
```cpp
try {
  auto response = http_client.CreateRequest()
    .get(url)
    .timeout(std::chrono::seconds(5))
    .perform();
    
  if (response->IsOk()) {
    return response->body();
  } else {
    LOG_WARNING() << "HTTP error: " << response->status_code();
    return std::nullopt;
  }
} catch (const clients::http::TimeoutException& e) {
  LOG_ERROR() << "Request timeout: " << e.what();
  throw;
} catch (const clients::http::NetworkProblemException& e) {
  LOG_ERROR() << "Network error: " << e.what();
  throw;
}
```

### Server Error Responses
```cpp
std::string HandleRequestThrow(
  const server::http::HttpRequest& request,
  server::request::RequestContext& context
) const override {
  try {
    return ProcessRequest(request, context);
  } catch (const ValidationError& e) {
    request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
    return fmt::format(R"({{"error": "{}"}})", e.what());
  } catch (const std::exception& e) {
    request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
    LOG_ERROR() << "Internal error: " << e.what();
    return R"({"error": "Internal server error"})";
  }
}
```

## Security Patterns

### Input Validation
```cpp
void ValidateRequest(const server::http::HttpRequest& request) {
  // Validate content type
  const auto content_type = request.GetHeader("Content-Type");
  if (content_type != "application/json") {
    throw ValidationError("Invalid content type");
  }
  
  // Validate request size
  if (request.RequestBody().size() > MAX_REQUEST_SIZE) {
    throw ValidationError("Request too large");
  }
  
  // Validate required headers
  if (!request.HasHeader("Authorization")) {
    throw AuthenticationError("Missing authorization header");
  }
}
```

### Rate Limiting
```yaml
server:
  middlewares:
    congestion-control:
      load-enabled: true
      upstreams-enabled: true
      upstreams-timeout: 35ms
```

### CORS Configuration
```cpp
void SetCorsHeaders(server::http::HttpRequest& request) {
  request.SetResponseHeader("Access-Control-Allow-Origin", "*");
  request.SetResponseHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
  request.SetResponseHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  request.SetResponseHeader("Access-Control-Max-Age", "86400");
}
```

## Monitoring and Logging

### Request Logging
```cpp
void LogRequest(
  const server::http::HttpRequest& request,
  const server::request::RequestContext& context
) {
  LOG_INFO() << "HTTP Request"
             << logging::LogExtra{
                  {"method", request.GetMethod()},
                  {"url", request.GetUrl()},
                  {"user_agent", request.GetHeader("User-Agent")},
                  {"request_id", context.GetRequestId()}
                };
}
```

### Metrics Collection
```cpp
// Custom metrics for HTTP operations
auto& http_requests_total = utils::statistics::GetMetric("http.requests.total");
auto& http_request_duration = utils::statistics::GetMetric("http.request.duration");

// Increment counters
http_requests_total.Inc({{"method", request.GetMethod()}, {"status", "200"}});

// Record timing
auto timer = http_request_duration.StartTimer();
// ... process request ...
timer.Stop();
```

## Testing Patterns

### Unit Testing HTTP Handlers
```cpp
#include <userver/utest/utest.hpp>
#include <userver/server/handlers/tests_control.hpp>

UTEST(HttpHandler, BasicRequest) {
  auto handler = std::make_unique<MyHandler>();
  
  auto request = server::http::HttpRequestBuilder{}
    .SetMethod(server::http::HttpMethod::kGet)
    .SetUrl("/api/test")
    .SetBody("{\"key\": \"value\"}")
    .Build();
    
  server::request::RequestContext context;
  auto response = handler->HandleRequestThrow(request, context);
  
  EXPECT_EQ(response, expected_response);
}
```

### Integration Testing with TestSuite
```python
async def test_http_endpoint(service_client):
    response = await service_client.post(
        '/api/endpoint',
        json={'data': 'test'},
        headers={'Content-Type': 'application/json'}
    )
    
    assert response.status == 200
    assert response.json()['result'] == 'success'
```

## Best Practices

### Connection Management
- Use connection pooling for HTTP clients
- Configure appropriate timeouts for different operations
- Implement proper retry strategies with exponential backoff
- Monitor connection pool metrics

### Performance Guidelines
- Enable HTTP/2 for better multiplexing
- Use streaming APIs for large responses
- Implement proper caching strategies
- Configure appropriate buffer sizes

### Security Considerations
- Always validate input data
- Use HTTPS in production environments
- Implement proper authentication and authorization
- Set security headers (HSTS, CSP, etc.)
- Rate limit requests to prevent abuse

### Error Handling
- Distinguish between client and server errors
- Provide meaningful error messages
- Log errors with sufficient context
- Implement circuit breaker patterns for external dependencies

## Cross-References

- **Memory Bank**: [`framework-core.md`](../../memory-bank/main/framework-core.md) - Core framework patterns
- **Memory Bank**: [`component-system.md`](../../memory-bank/main/component-system.md) - Component architecture
- **Memory Bank**: [`async-programming.md`](../../memory-bank/main/async-programming.md) - Asynchronous patterns
- **Rules**: [`error-handling.md`](../10-development/error-handling.md) - Error handling patterns
- **Rules**: [`logging.md`](../10-development/logging.md) - Logging best practices
- **Rules**: [`testing.md`](../10-development/testing.md) - Testing strategies