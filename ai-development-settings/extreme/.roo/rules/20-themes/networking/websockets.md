# WebSocket Implementation Patterns

## Overview

WebSocket implementation patterns for userver applications, covering real-time communication, connection management, message handling, and production-ready WebSocket services.

## Quality Tier
- **WebSocket**: Golden Tier

## Core Capabilities

- Full-duplex real-time communication
- Connection lifecycle management
- Message broadcasting and routing
- Authentication and authorization
- Connection pooling and scaling
- Graceful connection handling

## WebSocket Server Patterns

### Basic WebSocket Handler
```cpp
#include <userver/server/websocket/websocket_handler.hpp>

class ChatWebSocketHandler final : public server::websocket::WebSocketHandlerBase {
public:
  void Handle(server::websocket::WebSocketConnection& ws,
              server::request::RequestContext& context) const override {
    // Connection established
    LOG_INFO() << "WebSocket connection established";
    
    try {
      std::string message;
      while (ws.Recv(message)) {
        // Process incoming message
        ProcessMessage(ws, message, context);
      }
    } catch (const server::websocket::WebSocketError& e) {
      LOG_WARNING() << "WebSocket error: " << e.what();
    }
    
    LOG_INFO() << "WebSocket connection closed";
  }

private:
  void ProcessMessage(
    server::websocket::WebSocketConnection& ws,
    const std::string& message,
    server::request::RequestContext& context
  ) const {
    try {
      auto json_msg = formats::json::FromString(message);
      auto msg_type = json_msg["type"].As<std::string>();
      
      if (msg_type == "chat") {
        HandleChatMessage(ws, json_msg, context);
      } else if (msg_type == "ping") {
        HandlePingMessage(ws, json_msg);
      } else {
        SendError(ws, "Unknown message type");
      }
    } catch (const std::exception& e) {
      LOG_ERROR() << "Failed to process message: " << e.what();
      SendError(ws, "Invalid message format");
    }
  }
  
  void HandleChatMessage(
    server::websocket::WebSocketConnection& ws,
    const formats::json::Value& message,
    server::request::RequestContext& context
  ) const {
    auto user_id = GetUserId(context);
    auto chat_text = message["text"].As<std::string>();
    
    // Broadcast to all connected clients
    BroadcastMessage(user_id, chat_text);
    
    // Send acknowledgment
    formats::json::ValueBuilder response;
    response["type"] = "ack";
    response["message_id"] = message["id"];
    ws.Send(ToString(response.ExtractValue()));
  }
};
```

### WebSocket Configuration
```yaml
components_manager:
  components:
    server:
      listener:
        port: 8080
        task_processor: main-task-processor
      
    websocket-chat:
      path: /ws/chat
      method: GET
      task_processor: main-task-processor
      auth:
        types: [bearer]
      max-request-size: 1024
      max-response-size: 1024
```

### Connection Manager Pattern
```cpp
#include <userver/concurrent/variable.hpp>
#include <unordered_set>

class WebSocketConnectionManager {
public:
  using ConnectionId = std::string;
  using ConnectionPtr = std::shared_ptr<server::websocket::WebSocketConnection>;
  
  void AddConnection(const ConnectionId& id, ConnectionPtr connection) {
    auto connections = connections_.Lock();
    connections->emplace(id, std::move(connection));
    LOG_INFO() << "Added WebSocket connection: " << id 
               << ", total: " << connections->size();
  }
  
  void RemoveConnection(const ConnectionId& id) {
    auto connections = connections_.Lock();
    connections->erase(id);
    LOG_INFO() << "Removed WebSocket connection: " << id 
               << ", total: " << connections->size();
  }
  
  void BroadcastMessage(const std::string& message) {
    auto connections = connections_.Lock();
    
    for (auto it = connections->begin(); it != connections->end();) {
      try {
        it->second->Send(message);
        ++it;
      } catch (const server::websocket::WebSocketError& e) {
        LOG_WARNING() << "Failed to send to connection " << it->first 
                      << ": " << e.what();
        it = connections->erase(it);
      }
    }
  }
  
  void SendToConnection(const ConnectionId& id, const std::string& message) {
    auto connections = connections_.Lock();
    auto it = connections->find(id);
    
    if (it != connections->end()) {
      try {
        it->second->Send(message);
      } catch (const server::websocket::WebSocketError& e) {
        LOG_WARNING() << "Failed to send to connection " << id 
                      << ": " << e.what();
        connections->erase(it);
      }
    }
  }
  
  size_t GetConnectionCount() const {
    auto connections = connections_.Lock();
    return connections->size();
  }

private:
  concurrent::Variable<std::unordered_map<ConnectionId, ConnectionPtr>> connections_;
};
```

### Advanced WebSocket Handler with Connection Management
```cpp
class AdvancedWebSocketHandler final : public server::websocket::WebSocketHandlerBase {
public:
  AdvancedWebSocketHandler(WebSocketConnectionManager& manager)
    : connection_manager_(manager) {}

  void Handle(server::websocket::WebSocketConnection& ws,
              server::request::RequestContext& context) const override {
    
    // Authenticate connection
    auto user_id = AuthenticateConnection(context);
    if (user_id.empty()) {
      ws.Close(server::websocket::CloseStatusCode::kPolicyViolation, 
               "Authentication required");
      return;
    }
    
    // Generate connection ID
    auto connection_id = GenerateConnectionId(user_id);
    
    // Add to connection manager
    auto connection_ptr = std::shared_ptr<server::websocket::WebSocketConnection>(
      &ws, [](server::websocket::WebSocketConnection*) {
        // Custom deleter - don't actually delete
      });
    
    connection_manager_.AddConnection(connection_id, connection_ptr);
    
    // Send welcome message
    SendWelcomeMessage(ws, user_id);
    
    try {
      // Message handling loop
      std::string message;
      while (ws.Recv(message)) {
        ProcessMessage(ws, message, user_id, connection_id);
      }
    } catch (const server::websocket::WebSocketError& e) {
      LOG_INFO() << "WebSocket connection closed: " << e.what();
    }
    
    // Cleanup
    connection_manager_.RemoveConnection(connection_id);
    SendUserDisconnectedNotification(user_id);
  }

private:
  std::string AuthenticateConnection(server::request::RequestContext& context) const {
    try {
      auto auth_header = context.GetHttpRequest().GetHeader("Authorization");
      if (auth_header.empty()) {
        return {};
      }
      
      // Validate JWT token or API key
      return ValidateAuthToken(auth_header);
    } catch (const std::exception& e) {
      LOG_WARNING() << "Authentication failed: " << e.what();
      return {};
    }
  }
  
  void SendWelcomeMessage(server::websocket::WebSocketConnection& ws, 
                         const std::string& user_id) const {
    formats::json::ValueBuilder welcome;
    welcome["type"] = "welcome";
    welcome["user_id"] = user_id;
    welcome["server_time"] = utils::datetime::Now();
    
    ws.Send(ToString(welcome.ExtractValue()));
  }

  WebSocketConnectionManager& connection_manager_;
};
```

## Message Patterns

### Message Protocol Design
```cpp
namespace websocket_protocol {

enum class MessageType {
  kPing,
  kPong,
  kChat,
  kUserJoined,
  kUserLeft,
  kError,
  kAck
};

struct BaseMessage {
  MessageType type;
  std::string id;
  std::chrono::system_clock::time_point timestamp;
};

struct ChatMessage : BaseMessage {
  std::string user_id;
  std::string text;
  std::string channel;
};

struct ErrorMessage : BaseMessage {
  std::string error_code;
  std::string description;
};

// Message serialization
formats::json::Value SerializeMessage(const BaseMessage& message) {
  formats::json::ValueBuilder builder;
  builder["type"] = ToString(message.type);
  builder["id"] = message.id;
  builder["timestamp"] = utils::datetime::Timestring(message.timestamp);
  return builder.ExtractValue();
}

// Message deserialization
std::unique_ptr<BaseMessage> DeserializeMessage(const std::string& json_str) {
  auto json = formats::json::FromString(json_str);
  auto type_str = json["type"].As<std::string>();
  auto type = FromString<MessageType>(type_str);
  
  switch (type) {
    case MessageType::kChat:
      return DeserializeChatMessage(json);
    case MessageType::kPing:
      return DeserializePingMessage(json);
    // ... other message types
    default:
      throw std::runtime_error("Unknown message type: " + type_str);
  }
}

} // namespace websocket_protocol
```

### Message Broadcasting
```cpp
class MessageBroadcaster {
public:
  void BroadcastToChannel(const std::string& channel, 
                         const std::string& message) {
    auto subscribers = GetChannelSubscribers(channel);
    
    for (const auto& connection_id : subscribers) {
      connection_manager_.SendToConnection(connection_id, message);
    }
  }
  
  void BroadcastToUser(const std::string& user_id, 
                      const std::string& message) {
    auto user_connections = GetUserConnections(user_id);
    
    for (const auto& connection_id : user_connections) {
      connection_manager_.SendToConnection(connection_id, message);
    }
  }
  
  void BroadcastToAll(const std::string& message) {
    connection_manager_.BroadcastMessage(message);
  }

private:
  std::vector<std::string> GetChannelSubscribers(const std::string& channel) {
    auto subscriptions = channel_subscriptions_.Lock();
    auto it = subscriptions->find(channel);
    return (it != subscriptions->end()) ? it->second : std::vector<std::string>{};
  }
  
  concurrent::Variable<std::unordered_map<std::string, std::vector<std::string>>> 
    channel_subscriptions_;
  WebSocketConnectionManager& connection_manager_;
};
```

## Real-time Features

### Heartbeat/Ping-Pong Pattern
```cpp
class HeartbeatManager {
public:
  HeartbeatManager(std::chrono::seconds interval = std::chrono::seconds(30))
    : ping_interval_(interval) {}
    
  void StartHeartbeat(server::websocket::WebSocketConnection& ws,
                     const std::string& connection_id) {
    heartbeat_task_ = utils::PeriodicTask(
      "websocket_heartbeat",
      ping_interval_,
      [&ws, connection_id, this]() {
        SendPing(ws, connection_id);
      }
    );
  }
  
  void HandlePong(const std::string& connection_id) {
    auto now = std::chrono::steady_clock::now();
    last_pong_times_[connection_id] = now;
  }
  
  bool IsConnectionAlive(const std::string& connection_id) const {
    auto it = last_pong_times_.find(connection_id);
    if (it == last_pong_times_.end()) {
      return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - it->second;
    return elapsed < (ping_interval_ * 2); // Allow 2 missed pongs
  }

private:
  void SendPing(server::websocket::WebSocketConnection& ws,
               const std::string& connection_id) {
    try {
      formats::json::ValueBuilder ping;
      ping["type"] = "ping";
      ping["timestamp"] = utils::datetime::Now();
      
      ws.Send(ToString(ping.ExtractValue()));
    } catch (const server::websocket::WebSocketError& e) {
      LOG_WARNING() << "Failed to send ping to " << connection_id 
                    << ": " << e.what();
    }
  }
  
  std::chrono::seconds ping_interval_;
  utils::PeriodicTask heartbeat_task_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_pong_times_;
};
```

### Subscription Management
```cpp
class SubscriptionManager {
public:
  void Subscribe(const std::string& connection_id, 
                const std::string& channel) {
    auto subscriptions = subscriptions_.Lock();
    (*subscriptions)[channel].insert(connection_id);
    
    LOG_INFO() << "Connection " << connection_id 
               << " subscribed to channel " << channel;
  }
  
  void Unsubscribe(const std::string& connection_id, 
                  const std::string& channel) {
    auto subscriptions = subscriptions_.Lock();
    auto it = subscriptions->find(channel);
    if (it != subscriptions->end()) {
      it->second.erase(connection_id);
      if (it->second.empty()) {
        subscriptions->erase(it);
      }
    }
  }
  
  void UnsubscribeAll(const std::string& connection_id) {
    auto subscriptions = subscriptions_.Lock();
    for (auto it = subscriptions->begin(); it != subscriptions->end();) {
      it->second.erase(connection_id);
      if (it->second.empty()) {
        it = subscriptions->erase(it);
      } else {
        ++it;
      }
    }
  }
  
  std::vector<std::string> GetSubscribers(const std::string& channel) const {
    auto subscriptions = subscriptions_.Lock();
    auto it = subscriptions->find(channel);
    if (it != subscriptions->end()) {
      return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
  }

private:
  concurrent::Variable<std::unordered_map<std::string, std::unordered_set<std::string>>> 
    subscriptions_;
};
```

## Authentication and Authorization

### JWT-based Authentication
```cpp
class WebSocketAuthenticator {
public:
  struct AuthResult {
    bool success;
    std::string user_id;
    std::vector<std::string> permissions;
    std::string error_message;
  };
  
  AuthResult AuthenticateConnection(const server::http::HttpRequest& request) const {
    try {
      // Check for token in query parameters
      auto token = request.GetArg("token");
      if (token.empty()) {
        // Check Authorization header
        auto auth_header = request.GetHeader("Authorization");
        if (auth_header.starts_with("Bearer ")) {
          token = auth_header.substr(7);
        }
      }
      
      if (token.empty()) {
        return {false, {}, {}, "No authentication token provided"};
      }
      
      return ValidateJwtToken(token);
    } catch (const std::exception& e) {
      return {false, {}, {}, "Authentication failed: " + std::string(e.what())};
    }
  }
  
  bool HasPermission(const std::string& user_id, 
                    const std::string& permission) const {
    auto user_permissions = GetUserPermissions(user_id);
    return std::find(user_permissions.begin(), user_permissions.end(), permission) 
           != user_permissions.end();
  }

private:
  AuthResult ValidateJwtToken(const std::string& token) const {
    // JWT validation logic
    auto decoded = jwt::decode(token);
    auto verifier = jwt::verify()
      .allow_algorithm(jwt::algorithm::hs256{jwt_secret_})
      .with_issuer("userver-app");
      
    verifier.verify(decoded);
    
    auto user_id = decoded.get_payload_claim("sub").as_string();
    auto permissions_claim = decoded.get_payload_claim("permissions");
    
    std::vector<std::string> permissions;
    if (!permissions_claim.is_null()) {
      for (const auto& perm : permissions_claim.as_array()) {
        permissions.push_back(perm.as_string());
      }
    }
    
    return {true, user_id, permissions, {}};
  }
  
  std::string jwt_secret_;
};
```

## Error Handling and Recovery

### Connection Error Handling
```cpp
class WebSocketErrorHandler {
public:
  void HandleConnectionError(
    server::websocket::WebSocketConnection& ws,
    const server::websocket::WebSocketError& error,
    const std::string& connection_id
  ) {
    LOG_ERROR() << "WebSocket error for connection " << connection_id 
                << ": " << error.what();
    
    switch (error.GetCode()) {
      case server::websocket::WebSocketError::kConnectionClosed:
        HandleConnectionClosed(connection_id);
        break;
        
      case server::websocket::WebSocketError::kProtocolError:
        HandleProtocolError(ws, connection_id);
        break;
        
      case server::websocket::WebSocketError::kMessageTooLarge:
        HandleMessageTooLarge(ws, connection_id);
        break;
        
      default:
        HandleGenericError(ws, connection_id, error);
        break;
    }
  }

private:
  void HandleConnectionClosed(const std::string& connection_id) {
    // Cleanup resources
    connection_manager_.RemoveConnection(connection_id);
    subscription_manager_.UnsubscribeAll(connection_id);
  }
  
  void HandleProtocolError(server::websocket::WebSocketConnection& ws,
                          const std::string& connection_id) {
    SendErrorMessage(ws, "protocol_error", "WebSocket protocol violation");
    ws.Close(server::websocket::CloseStatusCode::kProtocolError, 
             "Protocol error");
  }
  
  void SendErrorMessage(server::websocket::WebSocketConnection& ws,
                       const std::string& error_code,
                       const std::string& description) {
    try {
      formats::json::ValueBuilder error;
      error["type"] = "error";
      error["error_code"] = error_code;
      error["description"] = description;
      
      ws.Send(ToString(error.ExtractValue()));
    } catch (...) {
      // Ignore errors when sending error messages
    }
  }
  
  WebSocketConnectionManager& connection_manager_;
  SubscriptionManager& subscription_manager_;
};
```

## Performance and Scaling

### Connection Limits
```yaml
server:
  max-connections: 10000
  connection-timeout: 60s
  
websocket-handler:
  max-message-size: 65536
  ping-interval: 30s
  pong-timeout: 10s
```

### Memory Management
```cpp
class WebSocketMemoryManager {
public:
  void SetConnectionLimit(size_t limit) {
    max_connections_ = limit;
  }
  
  bool CanAcceptConnection() const {
    return connection_manager_.GetConnectionCount() < max_connections_;
  }
  
  void MonitorMemoryUsage() {
    auto memory_usage = GetCurrentMemoryUsage();
    if (memory_usage > memory_threshold_) {
      LOG_WARNING() << "High memory usage: " << memory_usage 
                    << " bytes, closing idle connections";
      CloseIdleConnections();
    }
  }

private:
  void CloseIdleConnections() {
    // Implementation to close connections that haven't sent messages recently
    auto now = std::chrono::steady_clock::now();
    auto idle_threshold = std::chrono::minutes(5);
    
    // Close connections idle for more than threshold
    connection_manager_.CloseIdleConnections(now - idle_threshold);
  }
  
  size_t max_connections_ = 1000;
  size_t memory_threshold_ = 1024 * 1024 * 1024; // 1GB
  WebSocketConnectionManager& connection_manager_;
};
```

## Testing Patterns

### WebSocket Client for Testing
```cpp
#include <userver/clients/websocket/client.hpp>

class WebSocketTestClient {
public:
  WebSocketTestClient(const std::string& url) : url_(url) {}
  
  void Connect() {
    client_ = std::make_unique<clients::websocket::Client>(url_);
    client_->Connect();
  }
  
  void SendMessage(const std::string& message) {
    client_->Send(message);
  }
  
  std::string ReceiveMessage(std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    return client_->Receive(timeout);
  }
  
  void Close() {
    if (client_) {
      client_->Close();
    }
  }

private:
  std::string url_;
  std::unique_ptr<clients::websocket::Client> client_;
};

// Unit test example
UTEST(WebSocketHandler, ChatMessage) {
  WebSocketTestClient client("ws://localhost:8080/ws/chat?token=" + test_token);
  client.Connect();
  
  // Send chat message
  formats::json::ValueBuilder message;
  message["type"] = "chat";
  message["text"] = "Hello, World!";
  message["channel"] = "general";
  
  client.SendMessage(ToString(message.ExtractValue()));
  
  // Receive acknowledgment
  auto response = client.ReceiveMessage();
  auto response_json = formats::json::FromString(response);
  
  EXPECT_EQ(response_json["type"].As<std::string>(), "ack");
  
  client.Close();
}
```

## Best Practices

### Connection Management
- Implement proper connection limits
- Use heartbeat/ping-pong for connection health
- Clean up resources on disconnection
- Handle network interruptions gracefully

### Message Handling
- Validate all incoming messages
- Implement message size limits
- Use structured message protocols
- Handle malformed messages gracefully

### Security Guidelines
- Always authenticate WebSocket connections
- Validate message permissions
- Implement rate limiting
- Use secure WebSocket (WSS) in production

### Performance Optimization
- Use connection pooling
- Implement efficient broadcasting
- Monitor memory usage
- Close idle connections

### Error Handling
- Implement comprehensive error handling
- Provide meaningful error messages
- Log errors with sufficient context
- Handle partial message scenarios

## Cross-References

- **Memory Bank**: [`websocket-patterns.md`](../../memory-bank/specialized/websocket-advanced/websocket-patterns.md) - Advanced WebSocket patterns
- **Memory Bank**: [`framework-core.md`](../../memory-bank/main/framework-core.md) - Core framework patterns
- **Rules**: [`http-https.md`](./http-https.md) - HTTP/HTTPS patterns
- **Rules**: [`network-security.md`](./network-security.md) - Network security patterns
- **Rules**: [`error-handling.md`](../10-development/error-handling.md) - Error handling patterns
- **Rules**: [`testing.md`](../10-development/testing.md) - Testing strategies