# gRPC Implementation Patterns

## Overview

Comprehensive gRPC implementation patterns for userver applications, covering client and server patterns, streaming, middleware development, and production-ready configurations.

## Quality Tier
- **gRPC**: Platinum Tier

## Core Capabilities

- Asynchronous gRPC clients and services
- Connection caching and reusing
- Timeout and deadline management
- Automatic authentication via middlewares
- Deadline propagation support
- Comprehensive metrics collection
- Cancellation support

## Installation and Setup

### CMake Configuration
```cmake
userver_add_grpc_library(${PROJECT_NAME}-proto PROTOS samples/greeter.proto)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC ${PROJECT_NAME}-proto)
```

### Component Registration
```cpp
// For gRPC clients
#include <userver/ugrpc/client/client_factory_component.hpp>

// For gRPC servers  
#include <userver/ugrpc/server/server_component.hpp>
```

## gRPC Client Patterns

### Client Factory Configuration
```yaml
components_manager:
  components:
    grpc-client-factory:
      task-processor: grpc-blocking-task-processor
      channel-args:
        grpc.keepalive_time_ms: 30000
        grpc.keepalive_timeout_ms: 5000
        grpc.keepalive_permit_without_calls: true
        grpc.http2.max_pings_without_data: 0
        grpc.http2.min_time_between_pings_ms: 10000
        grpc.http2.min_ping_interval_without_data_ms: 300000
```

### Client Creation and Usage
```cpp
#include <userver/ugrpc/client/client_factory.hpp>

class MyServiceClient {
public:
  MyServiceClient(const ugrpc::client::ClientFactory& factory)
    : client_(factory.MakeClient<samples::GreeterServiceClient>("greeter-service")) {}

  samples::GreetingResponse SayHello(const samples::GreetingRequest& request) {
    auto context = std::make_unique<grpc::ClientContext>();
    context->set_deadline(engine::Deadline::FromDuration(std::chrono::seconds(1)));
    
    auto call = client_.SayHello(request, std::move(context));
    return call.Finish();
  }

private:
  samples::GreeterServiceClient client_;
};
```

### Streaming Client Patterns

#### Unary Call
```cpp
auto call = client_.UnaryMethod(request, std::move(context));
auto response = call.Finish();
```

#### Server Streaming
```cpp
auto stream = client_.ServerStreamingMethod(request, std::move(context));
samples::Response response;
while (stream.Read(response)) {
  ProcessResponse(response);
}
```

#### Client Streaming
```cpp
auto stream = client_.ClientStreamingMethod(std::move(context));
for (const auto& request : requests) {
  stream.Write(request);
}
stream.WritesDone();
auto response = stream.Finish();
```

#### Bidirectional Streaming
```cpp
auto stream = client_.BidirectionalStreamingMethod(std::move(context));

// Start reading in background
auto read_task = utils::Async("read_responses", [&stream]() {
  samples::Response response;
  while (stream.Read(response)) {
    ProcessResponse(response);
  }
});

// Write requests
for (const auto& request : requests) {
  stream.Write(request);
}
stream.WritesDone();

read_task.Get();
auto final_response = stream.Finish();
```

### Client TLS Configuration
```yaml
grpc-client-factory:
  auth-type: ssl  # or 'insecure' for testing
  channel-args:
    grpc.ssl_target_name_override: "localhost"
```

## gRPC Server Patterns

### Server Configuration
```yaml
components_manager:
  components:
    grpc-server:
      port: 8091
      task-processor: grpc-blocking-task-processor
      channel-args:
        grpc.max_receive_message_length: 4194304
        grpc.max_send_message_length: 4194304
```

### Service Implementation
```cpp
#include <samples/greeter_service.usrv.pb.hpp>

class GreeterService final : public samples::GreeterServiceBase {
public:
  void SayHello(
    SayHelloCall& call,
    samples::GreetingRequest&& request
  ) override {
    samples::GreetingResponse response;
    response.set_greeting("Hello, " + request.name() + "!");
    
    call.Finish(response);
  }
  
  void SayManyHellos(
    SayManyHellosCall& call,
    samples::GreetingRequest&& request
  ) override {
    for (int i = 0; i < 5; ++i) {
      samples::GreetingResponse response;
      response.set_greeting(fmt::format("Hello #{}, {}!", i + 1, request.name()));
      call.Write(response);
    }
    call.Finish();
  }
};
```

### Service Registration
```cpp
#include <userver/ugrpc/server/service_component_base.hpp>

class GreeterServiceComponent final : public ugrpc::server::ServiceComponentBase {
public:
  static constexpr std::string_view kName = "greeter-service";

  GreeterServiceComponent(const components::ComponentConfig& config,
                         const components::ComponentContext& context)
    : ServiceComponentBase(config, context),
      service_(context) {
    RegisterService(service_);
  }

private:
  GreeterService service_;
};
```

### Server Streaming Patterns

#### Server Streaming
```cpp
void ServerStreamingMethod(
  ServerStreamingCall& call,
  samples::Request&& request
) override {
  for (const auto& item : GetDataItems(request)) {
    samples::Response response;
    PopulateResponse(response, item);
    call.Write(response);
  }
  call.Finish();
}
```

#### Client Streaming
```cpp
void ClientStreamingMethod(ClientStreamingCall& call) override {
  samples::Request request;
  std::vector<samples::Request> requests;
  
  while (call.Read(request)) {
    requests.push_back(request);
  }
  
  samples::Response response = ProcessRequests(requests);
  call.Finish(response);
}
```

#### Bidirectional Streaming
```cpp
void BidirectionalStreamingMethod(BidirectionalStreamingCall& call) override {
  samples::Request request;
  while (call.Read(request)) {
    samples::Response response = ProcessRequest(request);
    call.Write(response);
  }
  call.Finish();
}
```

### Server TLS Configuration
```yaml
grpc-server:
  tls:
    key: /path/to/private.key
    cert: /path/to/cert.crt
    ca: /path/to/ca.crt  # Optional for client cert verification
```

### Custom Server Credentials
```cpp
class GrpcServerConfigurator final : public components::ComponentBase {
public:
  GrpcServerConfigurator(const components::ComponentConfig& config,
                        const components::ComponentContext& context)
    : ComponentBase(config, context) {
    
    auto& server = context.FindComponent<ugrpc::server::ServerComponent>().GetServer();
    
    server.WithServerBuilder([](grpc::ServerBuilder& builder) {
      auto credentials = grpc::SslServerCredentials(GetSslOptions());
      builder.AddListeningPort("0.0.0.0:8091", credentials);
    });
  }
};
```

## Middleware Patterns

### Client Middleware Implementation
```cpp
#include <userver/ugrpc/client/middleware_base.hpp>

class AuthClientMiddleware final : public ugrpc::client::MiddlewareBase {
public:
  void Handle(ugrpc::client::MiddlewareCallContext& context) const override {
    auto& client_context = context.GetContext();
    
    // Add authentication metadata
    client_context.AddMetadata("authorization", "Bearer " + GetToken());
    
    // Call next middleware
    context.Next();
  }
};
```

### Server Middleware Implementation
```cpp
#include <userver/ugrpc/server/middleware_base.hpp>

class LoggingServerMiddleware final : public ugrpc::server::MiddlewareBase {
public:
  void Handle(ugrpc::server::MiddlewareCallContext& context) const override {
    const auto& call_context = context.GetCall().GetContext();
    
    LOG_INFO() << "gRPC call started: " << context.GetCall().GetMethodName()
               << logging::LogExtra{{"peer", call_context.peer()}};
    
    try {
      context.Next();
      LOG_INFO() << "gRPC call completed successfully";
    } catch (const std::exception& e) {
      LOG_ERROR() << "gRPC call failed: " << e.what();
      throw;
    }
  }
};
```

### Standard Middleware Configuration
```yaml
components_manager:
  components:
    # Client middlewares
    grpc-client-logging:
      log-level: info
    grpc-client-deadline-propagation: {}
    grpc-client-baggage: {}
    grpc-client-headers-propagator:
      headers: ["x-request-id", "x-trace-id"]
    
    # Server middlewares  
    grpc-server-logging:
      log-level: info
    grpc-server-deadline-propagation: {}
    grpc-server-congestion-control:
      load-enabled: true
    grpc-server-baggage: {}
    grpc-server-headers-propagator:
      headers: ["x-request-id", "x-trace-id"]
```

## Error Handling Patterns

### Client Error Handling
```cpp
#include <userver/ugrpc/client/exceptions.hpp>

try {
  auto call = client_.SayHello(request, std::move(context));
  auto response = call.Finish();
  return response;
} catch (const ugrpc::client::RpcError& e) {
  if (e.GetStatusCode() == grpc::StatusCode::DEADLINE_EXCEEDED) {
    LOG_WARNING() << "gRPC call timeout: " << e.what();
    throw TimeoutException();
  } else if (e.GetStatusCode() == grpc::StatusCode::UNAVAILABLE) {
    LOG_WARNING() << "Service unavailable: " << e.what();
    throw ServiceUnavailableException();
  }
  throw;
} catch (const ugrpc::client::RpcCancelledError& e) {
  LOG_INFO() << "gRPC call cancelled: " << e.what();
  throw;
}
```

### Server Error Handling
```cpp
void SayHello(SayHelloCall& call, samples::GreetingRequest&& request) override {
  try {
    ValidateRequest(request);
    auto response = ProcessRequest(request);
    call.Finish(response);
  } catch (const ValidationError& e) {
    call.FinishWithError({grpc::StatusCode::INVALID_ARGUMENT, e.what()});
  } catch (const std::exception& e) {
    LOG_ERROR() << "Internal error: " << e.what();
    call.FinishWithError({grpc::StatusCode::INTERNAL, "Internal server error"});
  }
}
```

## Compression Configuration

### Server Compression
```yaml
grpc-server:
  channel-args:
    grpc.default_compression_algorithm: 2  # GRPC_COMPRESS_GZIP
    grpc.default_compression_level: 1      # GRPC_COMPRESS_LEVEL_LOW
```

### Client Compression
```yaml
grpc-client-factory:
  channel-args:
    grpc.default_compression_algorithm: 2  # GRPC_COMPRESS_GZIP
```

## Generic API Patterns

### Generic Client
```cpp
#include <userver/ugrpc/client/generic_client.hpp>

class ProxyService {
public:
  ProxyService(const ugrpc::client::ClientFactory& factory)
    : generic_client_(factory.MakeGenericClient("proxy-target")) {}
    
  std::string ProxyCall(
    const std::string& service,
    const std::string& method,
    const std::string& request_data
  ) {
    auto context = std::make_unique<grpc::ClientContext>();
    context->set_deadline(engine::Deadline::FromDuration(std::chrono::seconds(5)));
    
    auto call = generic_client_.UnaryCall(
      "/" + service + "/" + method,
      request_data,
      std::move(context)
    );
    
    return call.Finish();
  }

private:
  ugrpc::client::GenericClient generic_client_;
};
```

### Generic Server
```cpp
#include <userver/ugrpc/server/generic_service_base.hpp>

class GenericProxyService final : public ugrpc::server::GenericServiceBase {
public:
  void Handle(ugrpc::server::GenericCall& call) override {
    const auto method = call.GetMethodName();
    const auto request_data = call.GetRequestData();
    
    // Forward to backend service
    auto response_data = ForwardToBackend(method, request_data);
    
    call.Finish(response_data);
  }
};
```

## Monitoring and Metrics

### Built-in Metrics
- **Client metrics**: `grpc.client.by-destination`
- **Server metrics**: `grpc.server.by-destination` and `grpc.server.total`

### Metric Labels
- `grpc_service`: Fully qualified service name
- `grpc_method`: Fully qualified method name  
- `grpc_destination`: `service/method`
- `grpc_destination_full`: `client_name/service/method` (client only)

### Custom Metrics
```cpp
void RecordCustomMetrics(const std::string& method, std::chrono::milliseconds duration) {
  auto& custom_timer = utils::statistics::GetMetric("grpc.custom.request_duration");
  custom_timer.Account(duration.count(), {{"method", method}});
  
  auto& custom_counter = utils::statistics::GetMetric("grpc.custom.requests_total");
  custom_counter.Inc({{"method", method}, {"status", "success"}});
}
```

## Logging Patterns

### Request/Response Logging
```cpp
void LogGrpcCall(
  const std::string& method,
  const google::protobuf::Message& request,
  const google::protobuf::Message& response
) {
  LOG_INFO() << "gRPC Call"
             << logging::LogExtra{
                  {"method", method},
                  {"request", MessageToJson(request)},
                  {"response", MessageToJson(response)}
                };
}
```

### Field Redaction
```protobuf
// In your .proto file
import "userver/field_options.proto";

message UserCredentials {
  string username = 1;
  string password = 2 [debug_redact = true];
  string secret_token = 3 [debug_redact = true];
}
```

### Log Level Configuration
```yaml
grpc-server:
  native-log-level: error  # error, info, debug

grpc-client-common:
  native-log-level: error
```

## Testing Patterns

### Unit Testing Services
```cpp
#include <userver/ugrpc/tests/service.hpp>

UTEST(GreeterService, SayHello) {
  ugrpc::tests::Service service;
  service.RegisterService(greeter_service_);
  service.StartServer();
  
  auto client = service.MakeClient<samples::GreeterServiceClient>();
  
  samples::GreetingRequest request;
  request.set_name("World");
  
  auto context = std::make_unique<grpc::ClientContext>();
  auto call = client.SayHello(request, std::move(context));
  auto response = call.Finish();
  
  EXPECT_EQ(response.greeting(), "Hello, World!");
}
```

### Integration Testing
```python
# In testsuite
async def test_grpc_service(grpc_client):
    request = greeter_pb2.GreetingRequest(name="Test")
    response = await grpc_client.SayHello(request)
    assert response.greeting == "Hello, Test!"
```

## Performance Optimization

### Connection Pool Configuration
```yaml
grpc-client-factory:
  channel-args:
    grpc.keepalive_time_ms: 30000
    grpc.keepalive_timeout_ms: 5000
    grpc.max_receive_message_length: 16777216
    grpc.max_send_message_length: 16777216
    grpc.http2.max_pings_without_data: 0
    grpc.http2.min_time_between_pings_ms: 10000
```

### Server Performance Tuning
```yaml
grpc-server:
  channel-args:
    grpc.max_concurrent_streams: 1000
    grpc.http2.max_frame_size: 16384
    grpc.http2.hpack_table_size: 4096
```

## Best Practices

### Client Guidelines
- Always set deadlines for RPC calls
- Use connection pooling and reuse clients
- Implement proper retry strategies
- Handle cancellation gracefully
- Use streaming for large data transfers

### Server Guidelines
- Always call `Finish()` or `FinishWithError()`
- Validate input parameters
- Implement proper error handling
- Use streaming for large responses
- Monitor resource usage

### Security Considerations
- Use TLS in production environments
- Implement proper authentication
- Validate all input data
- Use field redaction for sensitive data
- Monitor for suspicious activity

### Performance Guidelines
- Enable compression for large messages
- Use appropriate message sizes
- Implement connection pooling
- Monitor and tune channel arguments
- Use bidirectional streaming efficiently

## Cross-References

- **Memory Bank**: [`framework-core.md`](../../memory-bank/main/framework-core.md) - Core framework patterns
- **Memory Bank**: [`component-system.md`](../../memory-bank/main/component-system.md) - Component architecture
- **Memory Bank**: [`async-programming.md`](../../memory-bank/main/async-programming.md) - Asynchronous patterns
- **Rules**: [`http-https.md`](./http-https.md) - HTTP/HTTPS patterns
- **Rules**: [`network-security.md`](./network-security.md) - Network security patterns
- **Rules**: [`error-handling.md`](../10-development/error-handling.md) - Error handling patterns
- **Rules**: [`testing.md`](../10-development/testing.md) - Testing strategies