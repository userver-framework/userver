# Advanced WebSocket Patterns in Userver

## Overview

This document covers advanced WebSocket implementation patterns, real-time communication strategies, and sophisticated messaging architectures using the userver framework. These patterns enable building scalable, high-performance real-time applications.

## Core WebSocket Patterns

### Connection Management

#### Advanced Connection Pool
```cpp
#include <userver/server/websocket/websocket_handler.hpp>
#include <userver/engine/async/channel.hpp>

class AdvancedWebSocketManager {
public:
    struct ConnectionInfo {
        std::string connection_id;
        std::string user_id;
        std::string session_id;
        std::chrono::system_clock::time_point connected_at;
        std::map<std::string, std::string> metadata;
        std::atomic<bool> is_active{true};
    };
    
    struct MessageRoute {
        std::string pattern;
        std::function<engine::TaskWithResult<void>(
            const std::string&, const formats::json::Value&)> handler;
        bool requires_auth{true};
        std::chrono::milliseconds timeout{std::chrono::seconds(30)};
    };
    
    // Advanced connection lifecycle management
    class ConnectionManager {
    public:
        ConnectionManager(size_t max_connections = 10000)
            : max_connections_(max_connections) {}
        
        engine::TaskWithResult<std::string> RegisterConnection(
            server::websocket::WebSocketConnection& connection,
            const std::string& user_id) {
            
            if (connections_.size() >= max_connections_) {
                throw std::runtime_error("Maximum connections exceeded");
            }
            
            auto connection_id = GenerateConnectionId();
            
            ConnectionInfo info;
            info.connection_id = connection_id;
            info.user_id = user_id;
            info.session_id = GenerateSessionId();
            info.connected_at = std::chrono::system_clock::now();
            
            {
                std::lock_guard lock(connections_mutex_);
                connections_[connection_id] = std::make_shared<ConnectionInfo>(info);
                user_connections_[user_id].insert(connection_id);
            }
            
            // Start connection monitoring
            StartConnectionMonitoring(connection_id, connection);
            
            LOG_INFO() << "WebSocket connection registered: " << connection_id
                      << " for user: " << user_id;
            
            co_return connection_id;
        }
        
        engine::TaskWithResult<void> UnregisterConnection(const std::string& connection_id) {
            std::lock_guard lock(connections_mutex_);
            
            auto it = connections_.find(connection_id);
            if (it != connections_.end()) {
                auto user_id = it->second->user_id;
                
                // Remove from user connections
                auto user_it = user_connections_.find(user_id);
                if (user_it != user_connections_.end()) {
                    user_it->second.erase(connection_id);
                    if (user_it->second.empty()) {
                        user_connections_.erase(user_it);
                    }
                }
                
                connections_.erase(it);
                
                LOG_INFO() << "WebSocket connection unregistered: " << connection_id;
            }
        }
        
        // Broadcast message to multiple connections
        engine::TaskWithResult<void> BroadcastToUsers(
            const std::vector<std::string>& user_ids,
            const formats::json::Value& message) {
            
            std::vector<engine::TaskWithResult<void>> send_tasks;
            
            {
                std::shared_lock lock(connections_mutex_);
                
                for (const auto& user_id : user_ids) {
                    auto user_it = user_connections_.find(user_id);
                    if (user_it != user_connections_.end()) {
                        for (const auto& connection_id : user_it->second) {
                            auto conn_it = connections_.find(connection_id);
                            if (conn_it != connections_.end() && conn_it->second->is_active) {
                                send_tasks.push_back(
                                    SendMessageToConnection(connection_id, message));
                            }
                        }
                    }
                }
            }
            
            // Send messages concurrently
            for (auto& task : send_tasks) {
                co_await task;
            }
        }
        
        // Get connection statistics
        formats::json::Value GetConnectionStats() const {
            std::shared_lock lock(connections_mutex_);
            
            formats::json::ValueBuilder stats;
            stats["total_connections"] = connections_.size();
            stats["unique_users"] = user_connections_.size();
            
            // Connection age distribution
            auto now = std::chrono::system_clock::now();
            size_t recent_connections = 0;
            
            for (const auto& [id, info] : connections_) {
                auto age = std::chrono::duration_cast<std::chrono::minutes>(
                    now - info->connected_at);
                if (age < std::chrono::minutes(5)) {
                    recent_connections++;
                }
            }
            
            stats["recent_connections"] = recent_connections;
            return stats.ExtractValue();
        }
        
    private:
        void StartConnectionMonitoring(
            const std::string& connection_id,
            server::websocket::WebSocketConnection& connection) {
            
            engine::AsyncNoSpan([this, connection_id, &connection]() {
                MonitorConnection(connection_id, connection);
            });
        }
        
        engine::TaskWithResult<void> MonitorConnection(
            const std::string& connection_id,
            server::websocket::WebSocketConnection& connection) {
            
            while (true) {
                co_await engine::SleepFor(std::chrono::seconds(30));
                
                // Check if connection is still active
                if (!IsConnectionActive(connection_id)) {
                    break;
                }
                
                // Send ping to keep connection alive
                try {
                    co_await connection.SendPing();
                } catch (const std::exception& ex) {
                    LOG_WARNING() << "Failed to ping connection " << connection_id
                                 << ": " << ex.what();
                    MarkConnectionInactive(connection_id);
                    break;
                }
            }
            
            co_await UnregisterConnection(connection_id);
        }
        
        bool IsConnectionActive(const std::string& connection_id) const {
            std::shared_lock lock(connections_mutex_);
            auto it = connections_.find(connection_id);
            return it != connections_.end() && it->second->is_active;
        }
        
        void MarkConnectionInactive(const std::string& connection_id) {
            std::shared_lock lock(connections_mutex_);
            auto it = connections_.find(connection_id);
            if (it != connections_.end()) {
                it->second->is_active = false;
            }
        }
        
        engine::TaskWithResult<void> SendMessageToConnection(
            const std::string& connection_id,
            const formats::json::Value& message) {
            
            // Implementation would send message to specific connection
            // This is a placeholder for the actual WebSocket send operation
            LOG_DEBUG() << "Sending message to connection: " << connection_id;
        }
        
        std::string GenerateConnectionId() {
            static std::atomic<uint64_t> counter{0};
            return "conn_" + std::to_string(counter++);
        }
        
        std::string GenerateSessionId() {
            static std::atomic<uint64_t> counter{0};
            return "sess_" + std::to_string(counter++);
        }
        
        size_t max_connections_;
        
        mutable std::shared_mutex connections_mutex_;
        std::map<std::string, std::shared_ptr<ConnectionInfo>> connections_;
        std::map<std::string, std::set<std::string>> user_connections_;
    };
};
```

### Message Routing and Processing

#### Advanced Message Router
```cpp
class WebSocketMessageRouter {
public:
    struct RouteHandler {
        std::string pattern;
        std::function<engine::TaskWithResult<formats::json::Value>(
            const std::string&, const formats::json::Value&)> handler;
        bool requires_auth;
        std::vector<std::string> required_permissions;
        std::chrono::milliseconds timeout;
    };
    
    // Pattern-based message routing
    void RegisterRoute(const RouteHandler& route) {
        std::lock_guard lock(routes_mutex_);
        routes_.push_back(route);
    }
    
    engine::TaskWithResult<void> ProcessMessage(
        const std::string& connection_id,
        const std::string& raw_message) {
        
        try {
            auto message = formats::json::FromString(raw_message);
            auto message_type = message["type"].As<std::string>();
            
            // Find matching route
            auto route = FindMatchingRoute(message_type);
            if (!route) {
                co_await SendError(connection_id, "Unknown message type: " + message_type);
                co_return;
            }
            
            // Check authentication if required
            if (route->requires_auth) {
                auto auth_result = co_await CheckAuthentication(connection_id, message);
                if (!auth_result.is_authenticated) {
                    co_await SendError(connection_id, "Authentication required");
                    co_return;
                }
                
                // Check permissions
                if (!CheckPermissions(auth_result.user_id, route->required_permissions)) {
                    co_await SendError(connection_id, "Insufficient permissions");
                    co_return;
                }
            }
            
            // Process message with timeout
            auto response = co_await engine::WaitFor(
                route->handler(connection_id, message),
                route->timeout
            );
            
            // Send response if provided
            if (!response.IsEmpty()) {
                co_await SendResponse(connection_id, response);
            }
            
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Error processing WebSocket message: " << ex.what();
            co_await SendError(connection_id, "Internal server error");
        }
    }
    
private:
    struct AuthResult {
        bool is_authenticated;
        std::string user_id;
        std::vector<std::string> permissions;
    };
    
    std::optional<RouteHandler> FindMatchingRoute(const std::string& message_type) {
        std::shared_lock lock(routes_mutex_);
        
        for (const auto& route : routes_) {
            if (MatchesPattern(route.pattern, message_type)) {
                return route;
            }
        }
        
        return std::nullopt;
    }
    
    bool MatchesPattern(const std::string& pattern, const std::string& message_type) {
        // Simple pattern matching - could be enhanced with regex
        return pattern == message_type || pattern == "*";
    }
    
    engine::TaskWithResult<AuthResult> CheckAuthentication(
        const std::string& connection_id,
        const formats::json::Value& message) {
        
        AuthResult result;
        
        // Extract authentication token from message
        if (message.HasMember("auth_token")) {
            auto token = message["auth_token"].As<std::string>();
            
            // Validate token (implementation specific)
            auto validation_result = co_await ValidateAuthToken(token);
            result.is_authenticated = validation_result.is_valid;
            result.user_id = validation_result.user_id;
            result.permissions = validation_result.permissions;
        }
        
        co_return result;
    }
    
    bool CheckPermissions(const std::string& user_id,
                         const std::vector<std::string>& required_permissions) {
        // Implementation would check user permissions
        return true; // Simplified
    }
    
    engine::TaskWithResult<void> SendResponse(
        const std::string& connection_id,
        const formats::json::Value& response) {
        
        // Implementation would send response to WebSocket connection
        LOG_DEBUG() << "Sending response to " << connection_id;
    }
    
    engine::TaskWithResult<void> SendError(
        const std::string& connection_id,
        const std::string& error_message) {
        
        formats::json::ValueBuilder error_response;
        error_response["type"] = "error";
        error_response["message"] = error_message;
        error_response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        co_await SendResponse(connection_id, error_response.ExtractValue());
    }
    
    struct TokenValidationResult {
        bool is_valid;
        std::string user_id;
        std::vector<std::string> permissions;
    };
    
    engine::TaskWithResult<TokenValidationResult> ValidateAuthToken(
        const std::string& token) {
        
        // Implementation would validate JWT or other token format
        TokenValidationResult result;
        result.is_valid = !token.empty(); // Simplified
        result.user_id = "user123";
        result.permissions = {"read", "write"};
        
        co_return result;
    }
    
    mutable std::shared_mutex routes_mutex_;
    std::vector<RouteHandler> routes_;
};
```

## Real-Time Communication Patterns

### Pub/Sub Messaging

#### WebSocket Pub/Sub System
```cpp
class WebSocketPubSubSystem {
public:
    struct Subscription {
        std::string connection_id;
        std::string topic;
        std::map<std::string, std::string> filters;
        std::chrono::system_clock::time_point subscribed_at;
    };
    
    struct Message {
        std::string topic;
        formats::json::Value payload;
        std::map<std::string, std::string> metadata;
        std::chrono::system_clock::time_point timestamp;
        std::string message_id;
    };
    
    // Topic subscription management
    engine::TaskWithResult<void> Subscribe(
        const std::string& connection_id,
        const std::string& topic,
        const std::map<std::string, std::string>& filters = {}) {
        
        Subscription subscription;
        subscription.connection_id = connection_id;
        subscription.topic = topic;
        subscription.filters = filters;
        subscription.subscribed_at = std::chrono::system_clock::now();
        
        {
            std::lock_guard lock(subscriptions_mutex_);
            topic_subscriptions_[topic].push_back(subscription);
            connection_subscriptions_[connection_id].insert(topic);
        }
        
        LOG_INFO() << "Connection " << connection_id << " subscribed to topic: " << topic;
        
        // Send subscription confirmation
        formats::json::ValueBuilder confirmation;
        confirmation["type"] = "subscription_confirmed";
        confirmation["topic"] = topic;
        confirmation["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            subscription.subscribed_at.time_since_epoch()).count();
        
        co_await SendToConnection(connection_id, confirmation.ExtractValue());
    }
    
    engine::TaskWithResult<void> Unsubscribe(
        const std::string& connection_id,
        const std::string& topic) {
        
        {
            std::lock_guard lock(subscriptions_mutex_);
            
            // Remove from topic subscriptions
            auto topic_it = topic_subscriptions_.find(topic);
            if (topic_it != topic_subscriptions_.end()) {
                topic_it->second.erase(
                    std::remove_if(topic_it->second.begin(), topic_it->second.end(),
                                  [&connection_id](const Subscription& sub) {
                                      return sub.connection_id == connection_id;
                                  }),
                    topic_it->second.end());
                
                if (topic_it->second.empty()) {
                    topic_subscriptions_.erase(topic_it);
                }
            }
            
            // Remove from connection subscriptions
            auto conn_it = connection_subscriptions_.find(connection_id);
            if (conn_it != connection_subscriptions_.end()) {
                conn_it->second.erase(topic);
                if (conn_it->second.empty()) {
                    connection_subscriptions_.erase(conn_it);
                }
            }
        }
        
        LOG_INFO() << "Connection " << connection_id << " unsubscribed from topic: " << topic;
    }
    
    // Message publishing
    engine::TaskWithResult<void> Publish(const Message& message) {
        std::vector<std::string> target_connections;
        
        {
            std::shared_lock lock(subscriptions_mutex_);
            
            auto topic_it = topic_subscriptions_.find(message.topic);
            if (topic_it != topic_subscriptions_.end()) {
                for (const auto& subscription : topic_it->second) {
                    if (MatchesFilters(message, subscription.filters)) {
                        target_connections.push_back(subscription.connection_id);
                    }
                }
            }
        }
        
        // Send message to all matching subscribers
        std::vector<engine::TaskWithResult<void>> send_tasks;
        
        for (const auto& connection_id : target_connections) {
            send_tasks.push_back(SendMessageToSubscriber(connection_id, message));
        }
        
        // Wait for all sends to complete
        for (auto& task : send_tasks) {
            try {
                co_await task;
            } catch (const std::exception& ex) {
                LOG_WARNING() << "Failed to send message to subscriber: " << ex.what();
            }
        }
        
        LOG_DEBUG() << "Published message to " << target_connections.size() 
                   << " subscribers on topic: " << message.topic;
    }
    
    // Cleanup subscriptions for disconnected connection
    void CleanupConnection(const std::string& connection_id) {
        std::lock_guard lock(subscriptions_mutex_);
        
        // Remove from all topic subscriptions
        for (auto& [topic, subscriptions] : topic_subscriptions_) {
            subscriptions.erase(
                std::remove_if(subscriptions.begin(), subscriptions.end(),
                              [&connection_id](const Subscription& sub) {
                                  return sub.connection_id == connection_id;
                              }),
                subscriptions.end());
        }
        
        // Remove empty topics
        for (auto it = topic_subscriptions_.begin(); it != topic_subscriptions_.end();) {
            if (it->second.empty()) {
                it = topic_subscriptions_.erase(it);
            } else {
                ++it;
            }
        }
        
        // Remove connection subscriptions
        connection_subscriptions_.erase(connection_id);
    }
    
private:
    bool MatchesFilters(const Message& message,
                       const std::map<std::string, std::string>& filters) {
        
        if (filters.empty()) {
            return true; // No filters means match all
        }
        
        for (const auto& [key, value] : filters) {
            auto metadata_it = message.metadata.find(key);
            if (metadata_it == message.metadata.end() || metadata_it->second != value) {
                return false;
            }
        }
        
        return true;
    }
    
    engine::TaskWithResult<void> SendMessageToSubscriber(
        const std::string& connection_id,
        const Message& message) {
        
        formats::json::ValueBuilder notification;
        notification["type"] = "message";
        notification["topic"] = message.topic;
        notification["payload"] = message.payload;
        notification["message_id"] = message.message_id;
        notification["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            message.timestamp.time_since_epoch()).count();
        
        // Add metadata
        if (!message.metadata.empty()) {
            formats::json::ValueBuilder metadata_builder;
            for (const auto& [key, value] : message.metadata) {
                metadata_builder[key] = value;
            }
            notification["metadata"] = metadata_builder.ExtractValue();
        }
        
        co_await SendToConnection(connection_id, notification.ExtractValue());
    }
    
    engine::TaskWithResult<void> SendToConnection(
        const std::string& connection_id,
        const formats::json::Value& message) {
        
        // Implementation would send message to WebSocket connection
        LOG_DEBUG() << "Sending message to connection: " << connection_id;
    }
    
    mutable std::shared_mutex subscriptions_mutex_;
    std::map<std::string, std::vector<Subscription>> topic_subscriptions_;
    std::map<std::string, std::set<std::string>> connection_subscriptions_;
};
```

## Performance Optimization Patterns

### Message Batching and Compression

#### Efficient Message Handling
```cpp
class OptimizedWebSocketHandler {
public:
    struct BatchConfig {
        size_t max_batch_size{100};
        std::chrono::milliseconds max_batch_delay{std::chrono::milliseconds(50)};
        bool enable_compression{true};
        size_t compression_threshold{1024}; // bytes
    };
    
    // Message batching for improved throughput
    class MessageBatcher {
    public:
        MessageBatcher(const BatchConfig& config) : config_(config) {
            StartBatchProcessor();
        }
        
        engine::TaskWithResult<void> QueueMessage(
            const std::string& connection_id,
            const formats::json::Value& message) {
            
            {
                std::lock_guard lock(batch_mutex_);
                pending_messages_[connection_id].push_back({
                    message,
                    std::chrono::steady_clock::now()
                });
            }
            
            batch_cv_.notify_one();
        }
        
    private:
        struct PendingMessage {
            formats::json::Value message;
            std::chrono::steady_clock::time_point queued_at;
        };
        
        void StartBatchProcessor() {
            batch_processor_ = engine::AsyncNoSpan([this]() {
                ProcessBatches();
            });
        }
        
        engine::TaskWithResult<void> ProcessBatches() {
            while (true) {
                std::unique_lock lock(batch_mutex_);
                
                // Wait for messages or timeout
                co_await batch_cv_.wait_for(lock, config_.max_batch_delay, [this] {
                    return !pending_messages_.empty();
                });
                
                if (pending_messages_.empty()) {
                    continue;
                }
                
                // Process batches for each connection
                auto batches_to_send = std::move(pending_messages_);
                pending_messages_.clear();
                lock.unlock();
                
                std::vector<engine::TaskWithResult<void>> send_tasks;
                
                for (auto& [connection_id, messages] : batches_to_send) {
                    if (!messages.empty()) {
                        send_tasks.push_back(SendBatch(connection_id, std::move(messages)));
                    }
                }
                
                // Send all batches concurrently
                for (auto& task : send_tasks) {
                    co_await task;
                }
            }
        }
        
        engine::TaskWithResult<void> SendBatch(
            const std::string& connection_id,
            std::vector<PendingMessage> messages) {
            
            if (messages.size() == 1) {
                // Single message - send directly
                co_await SendSingleMessage(connection_id, messages[0].message);
            } else {
                // Multiple messages - create batch
                formats::json::ValueBuilder batch;
                batch["type"] = "batch";
                batch["count"] = messages.size();
                
                formats::json::ValueBuilder messages_array(formats::json::Type::kArray);
                for (const auto& msg : messages) {
                    messages_array.PushBack(msg.message);
                }
                batch["messages"] = messages_array.ExtractValue();
                
                co_await SendSingleMessage(connection_id, batch.ExtractValue());
            }
        }
        
        engine::TaskWithResult<void> SendSingleMessage(
            const std::string& connection_id,
            const formats::json::Value& message) {
            
            auto message_str = message.ToString();
            
            // Apply compression if beneficial
            if (config_.enable_compression && 
                message_str.size() > config_.compression_threshold) {
                
                auto compressed = CompressMessage(message_str);
                if (compressed.size() < message_str.size() * 0.8) {
                    // Compression saved at least 20%
                    co_await SendCompressedMessage(connection_id, compressed);
                    co_return;
                }
            }
            
            // Send uncompressed
            co_await SendRawMessage(connection_id, message_str);
        }
        
        std::string CompressMessage(const std::string& message) {
            // Implementation would use compression library (e.g., zlib, lz4)
            return message; // Placeholder
        }
        
        engine::TaskWithResult<void> SendCompressedMessage(
            const std::string& connection_id,
            const std::string& compressed_data) {
            
            // Implementation would send compressed data with appropriate headers
            LOG_DEBUG() << "Sending compressed message to: " << connection_id;
        }
        
        engine::TaskWithResult<void> SendRawMessage(
            const std::string& connection_id,
            const std::string& message) {
            
            // Implementation would send raw message
            LOG_DEBUG() << "Sending message to: " << connection_id;
        }
        
        BatchConfig config_;
        
        std::mutex batch_mutex_;
        engine::ConditionVariable batch_cv_;
        std::map<std::string, std::vector<PendingMessage>> pending_messages_;
        
        engine::TaskWithResult<void> batch_processor_;
    };
};
```

## Security and Authentication Patterns

### Secure WebSocket Implementation

#### Authentication and Authorization
```cpp
class SecureWebSocketHandler {
public:
    struct SecurityConfig {
        bool require_authentication{true};
        std::chrono::minutes token_expiry{std::chrono::minutes(60)};
        size_t max_message_size{1024 * 1024}; // 1MB
        std::chrono::milliseconds rate_limit_window{std::chrono::seconds(60)};
        size_t max_messages_per_window{100};
    };
    
    // Rate limiting per connection
    class RateLimiter {
    public:
        RateLimiter(const SecurityConfig& config) : config_(config) {}
        
        bool CheckRateLimit(const std::string& connection_id) {
            auto now = std::chrono::steady_clock::now();
            
            std::lock_guard lock(rate_data_mutex_);
            auto& rate_data = connection_rates_[connection_id];
            
            // Clean old entries
            rate_data.erase(
                std::remove_if(rate_data.begin(), rate_data.end(),
                              [this, now](const std::chrono::steady_clock::time_point& timestamp) {
                                  return now - timestamp > config_.rate_limit_window;
                              }),
                rate_data.end());
            
            // Check if under limit
            if (rate_data.size() >= config_.max_messages_per_window) {
                return false; // Rate limit exceeded
            }
            
            // Record this message
            rate_data.push_back(now);
            return true;
        }
        
        void CleanupConnection(const std::string& connection_id) {
            std::lock_guard lock(rate_data_mutex_);
            connection_rates_.erase(connection_id);
        }
        
    private:
        SecurityConfig config_;
        std::mutex rate_data_mutex_;
        std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> connection_rates_;
    };
    
    // Message validation and sanitization
    class MessageValidator {
    public:
        MessageValidator(const SecurityConfig& config) : config_(config) {}
        
        struct ValidationResult {
            bool is_valid;
            std::string error_message;
            formats::json::Value sanitized_message;
        };
        
        ValidationResult ValidateMessage(const std::string& raw_message) {
            ValidationResult result;
            
            // Check message size
            if (raw_message.size() > config_.max_message_size) {
                result.is_valid = false;
                result.error_message = "Message too large";
                return result;
            }
            
            // Parse JSON
            try {
                auto message = formats::json::FromString(raw_message);
                result.sanitized_message = SanitizeMessage(message);
                result.is_valid = true;
            } catch (const std::exception& ex) {
                result.is_valid = false;
                result.error_message = "Invalid JSON: " + std::string(ex.what());
            }
            
            return result;
        }
        
    private:
        formats::json::Value SanitizeMessage(const formats::json::Value& message) {
            // Implement message sanitization logic
            // - Remove potentially dangerous fields
            // - Validate field types and ranges
            // - Escape special characters
            
            return message; // Simplified
        }
        
        SecurityConfig config_;
    };
};
```

These advanced WebSocket patterns provide a comprehensive foundation for building sophisticated real-time applications with userver. They cover connection management, message routing, pub/sub systems, performance optimization, and security considerations essential for production-grade WebSocket implementations.