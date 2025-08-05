# Advanced TCP/UDP Networking Patterns in Userver

## Overview

This document covers advanced networking patterns for implementing TCP and UDP communication in userver-based services. These patterns help build robust, high-performance network applications.

## TCP Networking Patterns

### TCP Client Implementation

#### Basic TCP Client
```cpp
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/timer.hpp>

class TcpClient {
public:
    TcpClient(const std::string& host, int port)
        : endpoint_(engine::io::Sockaddr::MakeWithAnyIpv4(host, port)) {}
    
    std::string SendRequest(const std::string& request,
                           std::chrono::milliseconds timeout) {
        // Create socket
        engine::io::Socket socket{engine::io::AddrFamily::kInet,
                                 engine::io::SocketType::kStream};
        
        // Connect with timeout
        socket.Connect(endpoint_, engine::Deadline::FromDuration(timeout));
        
        // Send data
        socket.SendAll(request.data(), request.size(),
                      engine::Deadline::FromDuration(timeout));
        
        // Receive response
        std::string response(4096, '\0');
        auto bytes_received = socket.RecvSome(response.data(), response.size(),
                                            engine::Deadline::FromDuration(timeout));
        response.resize(bytes_received);
        
        return response;
    }
    
private:
    engine::io::Sockaddr endpoint_;
};
```

#### Connection Pooling for TCP
```cpp
class TcpConnectionPool {
public:
    struct Connection {
        engine::io::Socket socket;
        std::chrono::steady_clock::time_point last_used;
        bool is_healthy{true};
    };
    
    std::shared_ptr<Connection> Acquire(const std::string& host, int port) {
        std::lock_guard lock(mutex_);
        
        // Try to find an existing connection
        auto key = std::make_pair(host, port);
        auto& connections = pool_[key];
        
        for (auto it = connections.begin(); it != connections.end();) {
            if (IsConnectionHealthy(*it)) {
                auto connection = std::move(*it);
                connections.erase(it);
                return std::make_shared<Connection>(std::move(*connection));
            } else {
                it = connections.erase(it);
            }
        }
        
        // Create new connection
        auto connection = std::make_unique<Connection>();
        connection->socket = engine::io::Socket{engine::io::AddrFamily::kInet,
                                               engine::io::SocketType::kStream};
        auto endpoint = engine::io::Sockaddr::MakeWithAnyIpv4(host, port);
        connection->socket.Connect(endpoint);
        connection->last_used = std::chrono::steady_clock::now();
        
        return std::make_shared<Connection>(std::move(*connection));
    }
    
    void Release(std::shared_ptr<Connection> connection,
                const std::string& host, int port) {
        if (!IsConnectionHealthy(connection)) {
            return; // Don't return broken connections to pool
        }
        
        connection->last_used = std::chrono::steady_clock::now();
        
        std::lock_guard lock(mutex_);
        auto key = std::make_pair(host, port);
        pool_[key].push_back(std::move(connection));
    }
    
private:
    bool IsConnectionHealthy(const std::shared_ptr<Connection>& connection) {
        if (!connection || !connection->is_healthy) {
            return false;
        }
        
        // Check if connection is still alive (implementation depends on protocol)
        return true;
    }
    
    std::mutex mutex_;
    std::map<std::pair<std::string, int>, std::vector<std::shared_ptr<Connection>>> pool_;
};
```

### TCP Server Implementation

#### Basic TCP Server
```cpp
class TcpServer {
public:
    TcpServer(int port) : port_(port) {}
    
    void Start() {
        // Create listening socket
        listener_ = engine::io::Socket{engine::io::AddrFamily::kInet,
                                      engine::io::SocketType::kStream};
        
        auto endpoint = engine::io::Sockaddr::MakeWithAnyIpv4("0.0.0.0", port_);
        listener_.Bind(endpoint);
        listener_.Listen(128);
        
        // Accept connections in a loop
        while (!should_stop_) {
            try {
                auto client_socket = listener_.Accept();
                engine::AsyncNoSpan([this, socket = std::move(client_socket)]() mutable {
                    HandleClient(std::move(socket));
                });
            } catch (const std::exception& ex) {
                LOG_ERROR() << "Error accepting connection: " << ex.what();
            }
        }
    }
    
    void Stop() {
        should_stop_ = true;
        if (listener_.IsValid()) {
            listener_.Close();
        }
    }
    
private:
    void HandleClient(engine::io::Socket socket) {
        try {
            while (true) {
                // Read request
                std::string request(4096, '\0');
                auto bytes_received = socket.RecvSome(request.data(), request.size());
                if (bytes_received == 0) {
                    break; // Connection closed
                }
                request.resize(bytes_received);
                
                // Process request
                auto response = ProcessRequest(request);
                
                // Send response
                socket.SendAll(response.data(), response.size());
            }
        } catch (const std::exception& ex) {
            LOG_WARNING() << "Error handling client: " << ex.what();
        }
    }
    
    std::string ProcessRequest(const std::string& request) {
        // Implement request processing logic
        return "Response to: " + request;
    }
    
    int port_;
    engine::io::Socket listener_;
    std::atomic<bool> should_stop_{false};
};
```

#### Protocol-Specific Server
```cpp
class ProtocolServer {
public:
    class ProtocolHandler {
    public:
        virtual ~ProtocolHandler() = default;
        virtual std::string HandleMessage(const std::string& message) = 0;
    };
    
    void SetProtocolHandler(std::unique_ptr<ProtocolHandler> handler) {
        handler_ = std::move(handler);
    }
    
    void HandleClient(engine::io::Socket socket) {
        if (!handler_) {
            LOG_ERROR() << "No protocol handler set";
            return;
        }
        
        try {
            ProtocolBuffer buffer;
            
            while (true) {
                // Read data into buffer
                std::string chunk(1024, '\0');
                auto bytes_received = socket.RecvSome(chunk.data(), chunk.size());
                if (bytes_received == 0) {
                    break; // Connection closed
                }
                chunk.resize(bytes_received);
                buffer.Append(chunk);
                
                // Process complete messages
                while (auto message = buffer.ExtractCompleteMessage()) {
                    auto response = handler_->HandleMessage(*message);
                    socket.SendAll(response.data(), response.size());
                }
            }
        } catch (const std::exception& ex) {
            LOG_WARNING() << "Error handling client: " << ex.what();
        }
    }
    
private:
    class ProtocolBuffer {
    public:
        void Append(const std::string& data) {
            buffer_ += data;
        }
        
        std::optional<std::string> ExtractCompleteMessage() {
            // Implement protocol-specific message extraction
            // This is a simple example for length-prefixed messages
            if (buffer_.size() < sizeof(uint32_t)) {
                return std::nullopt;
            }
            
            uint32_t message_length;
            std::memcpy(&message_length, buffer_.data(), sizeof(message_length));
            
            if (buffer_.size() < sizeof(uint32_t) + message_length) {
                return std::nullopt;
            }
            
            std::string message(buffer_.data() + sizeof(uint32_t), message_length);
            buffer_.erase(0, sizeof(uint32_t) + message_length);
            
            return message;
        }
        
    private:
        std::string buffer_;
    };
    
    std::unique_ptr<ProtocolHandler> handler_;
};
```

## UDP Networking Patterns

### UDP Client Implementation

#### Basic UDP Client
```cpp
class UdpClient {
public:
    UdpClient(const std::string& host, int port) {
        socket_ = engine::io::Socket{engine::io::AddrFamily::kInet,
                                    engine::io::SocketType::kDgram};
        endpoint_ = engine::io::Sockaddr::MakeWithAnyIpv4(host, port);
    }
    
    std::string SendRequest(const std::string& request,
                           std::chrono::milliseconds timeout) {
        // Send data
        socket_.SendTo(request.data(), request.size(), endpoint_,
                      engine::Deadline::FromDuration(timeout));
        
        // Receive response
        std::string response(4096, '\0');
        engine::io::Sockaddr sender_endpoint;
        auto bytes_received = socket_.RecvFrom(response.data(), response.size(),
                                             sender_endpoint,
                                             engine::Deadline::FromDuration(timeout));
        response.resize(bytes_received);
        
        return response;
    }
    
private:
    engine::io::Socket socket_;
    engine::io::Sockaddr endpoint_;
};
```

### UDP Server Implementation

#### Basic UDP Server
```cpp
class UdpServer {
public:
    UdpServer(int port) : port_(port) {}
    
    void Start() {
        socket_ = engine::io::Socket{engine::io::AddrFamily::kInet,
                                    engine::io::SocketType::kDgram};
        
        auto endpoint = engine::io::Sockaddr::MakeWithAnyIpv4("0.0.0.0", port_);
        socket_.Bind(endpoint);
        
        // Process messages in a loop
        while (!should_stop_) {
            try {
                ProcessMessage();
            } catch (const std::exception& ex) {
                LOG_ERROR() << "Error processing UDP message: " << ex.what();
            }
        }
    }
    
    void Stop() {
        should_stop_ = true;
        if (socket_.IsValid()) {
            socket_.Close();
        }
    }
    
private:
    void ProcessMessage() {
        std::string buffer(4096, '\0');
        engine::io::Sockaddr sender_endpoint;
        
        auto bytes_received = socket_.RecvFrom(buffer.data(), buffer.size(),
                                             sender_endpoint);
        buffer.resize(bytes_received);
        
        // Process message
        auto response = ProcessRequest(buffer);
        
        // Send response
        socket_.SendTo(response.data(), response.size(), sender_endpoint);
    }
    
    std::string ProcessRequest(const std::string& request) {
        // Implement request processing logic
        return "Response to: " + request;
    }
    
    int port_;
    engine::io::Socket socket_;
    std::atomic<bool> should_stop_{false};
};
```

## Advanced Networking Patterns

### Connection Management

#### Health Checking
```cpp
class ConnectionHealthChecker {
public:
    struct HealthStatus {
        bool is_healthy{false};
        std::chrono::milliseconds latency{0};
        std::string last_error;
    };
    
    HealthStatus CheckTcpConnection(const std::string& host, int port) {
        HealthStatus status;
        
        try {
            auto start_time = std::chrono::steady_clock::now();
            
            engine::io::Socket socket{engine::io::AddrFamily::kInet,
                                     engine::io::SocketType::kStream};
            auto endpoint = engine::io::Sockaddr::MakeWithAnyIpv4(host, port);
            
            socket.Connect(endpoint, engine::Deadline::FromDuration(std::chrono::seconds(5)));
            
            auto end_time = std::chrono::steady_clock::now();
            status.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time);
            status.is_healthy = true;
        } catch (const std::exception& ex) {
            status.last_error = ex.what();
        }
        
        return status;
    }
    
    void StartPeriodicHealthChecks() {
        health_check_task_ = engine::AsyncNoSpan([this]() {
            while (!health_check_stop_) {
                CheckAllConnections();
                engine::SleepFor(std::chrono::minutes(1));
            }
        });
    }
    
    void StopPeriodicHealthChecks() {
        health_check_stop_ = true;
        if (health_check_task_.IsValid()) {
            health_check_task_.Get();
        }
    }
    
private:
    void CheckAllConnections() {
        std::lock_guard lock(connections_mutex_);
        for (const auto& connection : connections_) {
            auto status = CheckTcpConnection(connection.host, connection.port);
            connection.status = status;
        }
    }
    
    struct ConnectionInfo {
        std::string host;
        int port;
        HealthStatus status;
    };
    
    std::mutex connections_mutex_;
    std::vector<ConnectionInfo> connections_;
    engine::TaskWithResult<void> health_check_task_;
    std::atomic<bool> health_check_stop_{false};
};
```

### Protocol Design Patterns

#### Message Framing
```cpp
class MessageFramer {
public:
    // Length-prefixed framing
    static std::string FrameMessage(const std::string& message) {
        std::string framed;
        uint32_t length = static_cast<uint32_t>(message.size());
        
        framed.resize(sizeof(length) + message.size());
        std::memcpy(framed.data(), &length, sizeof(length));
        std::memcpy(framed.data() + sizeof(length), message.data(), message.size());
        
        return framed;
    }
    
    // Delimiter-based framing
    static std::string FrameMessageWithDelimiter(const std::string& message,
                                               char delimiter = '\n') {
        return message + delimiter;
    }
    
    // Fixed-size framing
    static std::string FrameFixedSizeMessage(const std::string& message,
                                           size_t fixed_size) {
        if (message.size() > fixed_size) {
            throw std::runtime_error("Message too large for fixed-size frame");
        }
        
        std::string framed(fixed_size, '\0');
        std::memcpy(framed.data(), message.data(), message.size());
        return framed;
    }
};
```

### Error Handling and Recovery

#### Connection Recovery
```cpp
class ResilientTcpClient {
public:
    ResilientTcpClient(const std::string& host, int port)
        : host_(host), port_(port) {}
    
    std::string SendRequest(const std::string& request,
                           std::chrono::milliseconds timeout,
                           int max_retries = 3) {
        for (int attempt = 0; attempt <= max_retries; ++attempt) {
            try {
                auto connection = GetConnection();
                auto response = SendWithConnection(*connection, request, timeout);
                ReturnConnection(std::move(connection));
                return response;
            } catch (const std::exception& ex) {
                LOG_WARNING() << "Attempt " << (attempt + 1) << " failed: " << ex.what();
                
                if (attempt == max_retries) {
                    throw; // Last attempt, re-throw
                }
                
                // Exponential backoff
                auto delay = std::chrono::milliseconds(100 * (1 << attempt));
                engine::SleepFor(delay);
            }
        }
        
        throw std::runtime_error("Unreachable code");
    }
    
private:
    std::unique_ptr<engine::io::Socket> GetConnection() {
        std::lock_guard lock(connection_mutex_);
        
        if (!available_connections_.empty()) {
            auto connection = std::move(available_connections_.back());
            available_connections_.pop_back();
            return connection;
        }
        
        // Create new connection
        auto connection = std::make_unique<engine::io::Socket>(
            engine::io::AddrFamily::kInet,
            engine::io::SocketType::kStream
        );
        
        auto endpoint = engine::io::Sockaddr::MakeWithAnyIpv4(host_, port_);
        connection->Connect(endpoint);
        
        return connection;
    }
    
    void ReturnConnection(std::unique_ptr<engine::io::Socket> connection) {
        if (IsConnectionHealthy(connection)) {
            std::lock_guard lock(connection_mutex_);
            available_connections_.push_back(std::move(connection));
        }
        // Connection is dropped if unhealthy
    }
    
    bool IsConnectionHealthy(const std::unique_ptr<engine::io::Socket>& socket) {
        if (!socket || !socket->IsValid()) {
            return false;
        }
        
        // Try a simple operation to check health
        try {
            // This is a simplified check - actual implementation depends on protocol
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    std::string SendWithConnection(engine::io::Socket& socket,
                                  const std::string& request,
                                  std::chrono::milliseconds timeout) {
        socket.SendAll(request.data(), request.size(),
                      engine::Deadline::FromDuration(timeout));
        
        std::string response(4096, '\0');
        auto bytes_received = socket.RecvSome(response.data(), response.size(),
                                            engine::Deadline::FromDuration(timeout));
        response.resize(bytes_received);
        
        return response;
    }
    
    std::string host_;
    int port_;
    std::mutex connection_mutex_;
    std::vector<std::unique_ptr<engine::io::Socket>> available_connections_;
};
```

### Performance Optimization

#### Buffer Management
```cpp
class BufferPool {
public:
    class Buffer {
    public:
        Buffer(std::vector<char> data, BufferPool* pool)
            : data_(std::move(data)), pool_(pool) {}
        
        ~Buffer() {
            if (pool_) {
                pool_->ReturnBuffer(std::move(data_));
            }
        }
        
        char* data() { return data_.data(); }
        size_t size() const { return data_.size(); }
        
        // Disable copy, enable move
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) = default;
        Buffer& operator=(Buffer&&) = default;
        
    private:
        std::vector<char> data_;
        BufferPool* pool_;
    };
    
    std::unique_ptr<Buffer> GetBuffer(size_t min_size) {
        std::lock_guard lock(mutex_);
        
        // Find appropriately sized buffer
        for (auto it = buffers_.begin(); it != buffers_.end(); ++it) {
            if (it->size() >= min_size) {
                auto buffer = std::make_unique<Buffer>(std::move(*it), this);
                buffers_.erase(it);
                return buffer;
            }
        }
        
        // Create new buffer
        return std::make_unique<Buffer>(std::vector<char>(min_size), this);
    }
    
private:
    void ReturnBuffer(std::vector<char> buffer) {
        std::lock_guard lock(mutex_);
        buffers_.push_back(std::move(buffer));
    }
    
    std::mutex mutex_;
    std::vector<std::vector<char>> buffers_;
};
```

These networking patterns provide a solid foundation for building robust TCP and UDP applications using the userver framework. They cover connection management, error handling, protocol design, and performance optimization techniques that are essential for production-grade network services.