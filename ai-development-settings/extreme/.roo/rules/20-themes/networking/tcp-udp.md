# TCP/UDP Networking Patterns

## Overview

Low-level TCP and UDP networking patterns for userver applications, covering socket programming, connection management, protocol implementation, and high-performance networking.

## Core Components

### Socket Management
- [`engine::io::Socket`](https://userver.tech/d8/d32/classengine_1_1io_1_1Socket.html) - Low-level socket operations
- [`engine::io::Sockaddr`](https://userver.tech/d1/d32/classengine_1_1io_1_1Sockaddr.html) - Socket address management
- [`engine::io::TlsWrapper`](https://userver.tech/d5/d32/classengine_1_1io_1_1TlsWrapper.html) - TLS encryption for sockets
- [`engine::Deadline`](https://userver.tech/d2/d32/classengine_1_1Deadline.html) - Timeout management

### Task System Integration
- [`engine::Task`](https://userver.tech/d3/d32/classengine_1_1Task.html) - Asynchronous task execution
- [`utils::Async`](https://userver.tech/d4/d32/namespaceutils.html#async) - Async operation utilities
- [`concurrent::Variable`](https://userver.tech/d5/d32/classconcurrent_1_1Variable.html) - Thread-safe variables

## TCP Server Implementation

### Basic TCP Server
```cpp
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/async.hpp>

class TcpServer {
public:
  explicit TcpServer(const engine::io::Sockaddr& listen_addr)
    : listen_addr_(listen_addr) {}
  
  void Start() {
    listen_socket_ = engine::io::Socket::Create(
      listen_addr_.Domain(),
      engine::io::SocketType::kStream
    );
    
    listen_socket_.Bind(listen_addr_);
    listen_socket_.Listen(128); // backlog
    
    LOG_INFO() << "TCP server listening on " << listen_addr_.PrimaryAddressString();
    
    while (!should_stop_) {
      try {
        auto client_socket = listen_socket_.Accept({});
        
        // Handle client in separate task
        utils::Async("client_handler", [this, socket = std::move(client_socket)]() mutable {
          HandleClient(std::move(socket));
        }).Detach();
        
      } catch (const std::exception& e) {
        LOG_ERROR() << "Accept failed: " << e.what();
      }
    }
  }
  
  void Stop() {
    should_stop_ = true;
    if (listen_socket_.IsValid()) {
      listen_socket_.Close();
    }
  }

private:
  void HandleClient(engine::io::Socket client_socket) {
    try {
      auto client_addr = client_socket.Getpeername();
      LOG_INFO() << "Client connected from " << client_addr.PrimaryAddressString();
      
      std::array<char, 4096> buffer;
      
      while (client_socket.IsValid()) {
        auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(30));
        
        auto bytes_received = client_socket.RecvSome(
          buffer.data(), buffer.size(), deadline
        );
        
        if (bytes_received == 0) {
          break; // Client disconnected
        }
        
        // Echo back to client
        client_socket.SendAll(buffer.data(), bytes_received, deadline);
      }
      
    } catch (const std::exception& e) {
      LOG_ERROR() << "Client handler error: " << e.what();
    }
  }
  
  engine::io::Sockaddr listen_addr_;
  engine::io::Socket listen_socket_;
  std::atomic<bool> should_stop_{false};
};
```

### Advanced TCP Server with Connection Management
```cpp
class AdvancedTcpServer {
public:
  struct Config {
    engine::io::Sockaddr listen_addr;
    size_t max_connections = 1000;
    std::chrono::seconds connection_timeout{60};
    size_t buffer_size = 65536;
    bool tcp_nodelay = true;
    bool keep_alive = true;
  };
  
  explicit AdvancedTcpServer(Config config) : config_(std::move(config)) {}
  
  void Start() {
    listen_socket_ = engine::io::Socket::Create(
      config_.listen_addr.Domain(),
      engine::io::SocketType::kStream
    );
    
    // Configure socket options
    listen_socket_.SetOption(engine::io::Socket::Option::kReuseAddr, 1);
    if (config_.tcp_nodelay) {
      listen_socket_.SetOption(engine::io::Socket::Option::kTcpNoDelay, 1);
    }
    if (config_.keep_alive) {
      listen_socket_.SetOption(engine::io::Socket::Option::kKeepAlive, 1);
    }
    
    listen_socket_.Bind(config_.listen_addr);
    listen_socket_.Listen(128);
    
    LOG_INFO() << "Advanced TCP server listening on " 
               << config_.listen_addr.PrimaryAddressString();
    
    // Start connection manager
    connection_manager_task_ = utils::Async("connection_manager", [this]() {
      ManageConnections();
    });
    
    // Accept connections
    while (!should_stop_) {
      try {
        if (GetActiveConnectionCount() >= config_.max_connections) {
          engine::SleepFor(std::chrono::milliseconds(100));
          continue;
        }
        
        auto client_socket = listen_socket_.Accept({});
        auto connection_id = GenerateConnectionId();
        
        RegisterConnection(connection_id, client_socket.Getpeername());
        
        utils::Async("client_handler", 
          [this, socket = std::move(client_socket), connection_id]() mutable {
            HandleClient(std::move(socket), connection_id);
          }
        ).Detach();
        
      } catch (const std::exception& e) {
        LOG_ERROR() << "Accept failed: " << e.what();
      }
    }
  }
  
  void Stop() {
    should_stop_ = true;
    if (listen_socket_.IsValid()) {
      listen_socket_.Close();
    }
    if (connection_manager_task_.IsValid()) {
      connection_manager_task_.SyncCancel();
    }
  }
  
  size_t GetActiveConnectionCount() const {
    auto connections = connections_.Lock();
    return connections->size();
  }

private:
  struct ConnectionInfo {
    engine::io::Sockaddr client_addr;
    std::chrono::steady_clock::time_point created_at;
    std::chrono::steady_clock::time_point last_activity;
  };
  
  void HandleClient(engine::io::Socket client_socket, const std::string& connection_id) {
    try {
      std::vector<char> buffer(config_.buffer_size);
      
      while (client_socket.IsValid() && !should_stop_) {
        auto deadline = engine::Deadline::FromDuration(config_.connection_timeout);
        
        auto bytes_received = client_socket.RecvSome(
          buffer.data(), buffer.size(), deadline
        );
        
        if (bytes_received == 0) {
          break; // Client disconnected
        }
        
        UpdateConnectionActivity(connection_id);
        
        // Process received data
        ProcessData(buffer.data(), bytes_received, client_socket, deadline);
      }
      
    } catch (const std::exception& e) {
      LOG_ERROR() << "Client handler error for " << connection_id << ": " << e.what();
    }
    
    UnregisterConnection(connection_id);
  }
  
  void ProcessData(const char* data, size_t size, 
                  engine::io::Socket& client_socket,
                  engine::Deadline deadline) {
    // Echo implementation - override for custom protocol
    client_socket.SendAll(data, size, deadline);
  }
  
  void ManageConnections() {
    while (!should_stop_) {
      engine::SleepFor(std::chrono::seconds(10));
      
      auto now = std::chrono::steady_clock::now();
      std::vector<std::string> expired_connections;
      
      {
        auto connections = connections_.Lock();
        for (const auto& [id, info] : *connections) {
          if (now - info.last_activity > config_.connection_timeout) {
            expired_connections.push_back(id);
          }
        }
      }
      
      for (const auto& id : expired_connections) {
        LOG_INFO() << "Closing expired connection: " << id;
        UnregisterConnection(id);
      }
    }
  }
  
  std::string GenerateConnectionId() {
    static std::atomic<uint64_t> counter{0};
    return "conn_" + std::to_string(counter.fetch_add(1));
  }
  
  void RegisterConnection(const std::string& id, const engine::io::Sockaddr& addr) {
    auto connections = connections_.Lock();
    auto now = std::chrono::steady_clock::now();
    (*connections)[id] = ConnectionInfo{addr, now, now};
  }
  
  void UpdateConnectionActivity(const std::string& id) {
    auto connections = connections_.Lock();
    auto it = connections->find(id);
    if (it != connections->end()) {
      it->second.last_activity = std::chrono::steady_clock::now();
    }
  }
  
  void UnregisterConnection(const std::string& id) {
    auto connections = connections_.Lock();
    connections->erase(id);
  }
  
  Config config_;
  engine::io::Socket listen_socket_;
  engine::TaskWithResult<void> connection_manager_task_;
  std::atomic<bool> should_stop_{false};
  concurrent::Variable<std::unordered_map<std::string, ConnectionInfo>> connections_;
};
```

## UDP Server Implementation

### Basic UDP Server
```cpp
class UdpServer {
public:
  explicit UdpServer(const engine::io::Sockaddr& bind_addr)
    : bind_addr_(bind_addr) {}
  
  void Start() {
    socket_ = engine::io::Socket::Create(
      bind_addr_.Domain(),
      engine::io::SocketType::kDgram
    );
    
    socket_.Bind(bind_addr_);
    
    LOG_INFO() << "UDP server listening on " << bind_addr_.PrimaryAddressString();
    
    std::array<char, 65536> buffer; // Max UDP packet size
    
    while (!should_stop_) {
      try {
        auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(1));
        
        engine::io::Sockaddr client_addr;
        auto bytes_received = socket_.RecvFrom(
          buffer.data(), buffer.size(), client_addr, deadline
        );
        
        if (bytes_received > 0) {
          // Process packet in separate task for better concurrency
          utils::Async("udp_handler", 
            [this, data = std::string(buffer.data(), bytes_received), client_addr]() {
              HandlePacket(data, client_addr);
            }
          ).Detach();
        }
        
      } catch (const engine::io::IoTimeout&) {
        // Timeout is expected, continue
      } catch (const std::exception& e) {
        LOG_ERROR() << "UDP receive error: " << e.what();
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
  void HandlePacket(const std::string& data, const engine::io::Sockaddr& client_addr) {
    try {
      // Echo back with prefix
      std::string response = "Echo: " + data;
      
      auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(5));
      socket_.SendTo(response.data(), response.size(), client_addr, deadline);
      
    } catch (const std::exception& e) {
      LOG_ERROR() << "UDP send error: " << e.what();
    }
  }
  
  engine::io::Sockaddr bind_addr_;
  engine::io::Socket socket_;
  std::atomic<bool> should_stop_{false};
};
```

### High-Performance UDP Server
```cpp
class HighPerformanceUdpServer {
public:
  struct Config {
    engine::io::Sockaddr bind_addr;
    size_t worker_threads = 4;
    size_t max_packet_size = 65536;
    size_t receive_buffer_size = 1048576; // 1MB
    std::chrono::milliseconds receive_timeout{1000};
  };
  
  explicit HighPerformanceUdpServer(Config config) : config_(std::move(config)) {}
  
  void Start() {
    socket_ = engine::io::Socket::Create(
      config_.bind_addr.Domain(),
      engine::io::SocketType::kDgram
    );
    
    // Configure socket for high performance
    socket_.SetOption(engine::io::Socket::Option::kReuseAddr, 1);
    socket_.SetOption(engine::io::Socket::Option::kReceiveBufferSize, 
                     config_.receive_buffer_size);
    
    socket_.Bind(config_.bind_addr);
    
    LOG_INFO() << "High-performance UDP server listening on " 
               << config_.bind_addr.PrimaryAddressString();
    
    // Start worker tasks
    for (size_t i = 0; i < config_.worker_threads; ++i) {
      worker_tasks_.push_back(
        utils::Async("udp_worker_" + std::to_string(i), [this]() {
          WorkerLoop();
        })
      );
    }
  }
  
  void Stop() {
    should_stop_ = true;
    if (socket_.IsValid()) {
      socket_.Close();
    }
    
    for (auto& task : worker_tasks_) {
      if (task.IsValid()) {
        task.SyncCancel();
      }
    }
    worker_tasks_.clear();
  }

private:
  void WorkerLoop() {
    std::vector<char> buffer(config_.max_packet_size);
    
    while (!should_stop_) {
      try {
        auto deadline = engine::Deadline::FromDuration(config_.receive_timeout);
        
        engine::io::Sockaddr client_addr;
        auto bytes_received = socket_.RecvFrom(
          buffer.data(), buffer.size(), client_addr, deadline
        );
        
        if (bytes_received > 0) {
          ProcessPacket(buffer.data(), bytes_received, client_addr);
        }
        
      } catch (const engine::io::IoTimeout&) {
        // Timeout is expected, continue
      } catch (const std::exception& e) {
        LOG_ERROR() << "UDP worker error: " << e.what();
      }
    }
  }
  
  void ProcessPacket(const char* data, size_t size, 
                    const engine::io::Sockaddr& client_addr) {
    // Custom packet processing logic
    // This example implements a simple echo with statistics
    
    packet_count_.fetch_add(1);
    bytes_received_.fetch_add(size);
    
    try {
      // Echo back
      auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(5));
      socket_.SendTo(data, size, client_addr, deadline);
      
      bytes_sent_.fetch_add(size);
      
    } catch (const std::exception& e) {
      LOG_ERROR() << "UDP send error: " << e.what();
      error_count_.fetch_add(1);
    }
  }
  
  Config config_;
  engine::io::Socket socket_;
  std::vector<engine::TaskWithResult<void>> worker_tasks_;
  std::atomic<bool> should_stop_{false};
  
  // Statistics
  std::atomic<uint64_t> packet_count_{0};
  std::atomic<uint64_t> bytes_received_{0};
  std::atomic<uint64_t> bytes_sent_{0};
  std::atomic<uint64_t> error_count_{0};
};
```

## TCP Client Implementation

### Basic TCP Client
```cpp
class TcpClient {
public:
  explicit TcpClient(const engine::io::Sockaddr& server_addr)
    : server_addr_(server_addr) {}
  
  void Connect() {
    socket_ = engine::io::Socket::Create(
      server_addr_.Domain(),
      engine::io::SocketType::kStream
    );
    
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(5));
    socket_.Connect(server_addr_, deadline);
  }
  
  std::string SendRequest(const std::string& request) {
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
    socket_.SendAll(request.data(), request.size(), deadline);
    
    std::array<char, 4096> buffer;
    auto bytes_received = socket_.RecvSome(buffer.data(), buffer.size(), deadline);
    return std::string(buffer.data(), bytes_received);
  }
  
  void Disconnect() {
    if (socket_.IsValid()) {
      socket_.Close();
    }
  }

private:
  engine::io::Sockaddr server_addr_;
  engine::io::Socket socket_;
};
```

### Connection Pool TCP Client
```cpp
class TcpConnectionPool {
public:
  struct Config {
    engine::io::Sockaddr server_addr;
    size_t max_connections = 10;
    std::chrono::seconds connection_timeout{30};
    std::chrono::seconds idle_timeout{60};
    bool tcp_nodelay = true;
  };
  
  explicit TcpConnectionPool(Config config) : config_(std::move(config)) {}
  
  class Connection {
  public:
    Connection(engine::io::Socket socket, TcpConnectionPool* pool)
      : socket_(std::move(socket)), pool_(pool) {}
    
    ~Connection() {
      if (pool_) {
        pool_->ReturnConnection(std::move(socket_));
      }
    }
    
    std::string SendRequest(const std::string& request) {
      auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
      socket_.SendAll(request.data(), request.size(), deadline);
      
      std::array<char, 4096> buffer;
      auto bytes_received = socket_.RecvSome(buffer.data(), buffer.size(), deadline);
      return std::string(buffer.data(), bytes_received);
    }
    
  private:
    engine::io::Socket socket_;
    TcpConnectionPool* pool_;
  };
  
  std::unique_ptr<Connection> GetConnection() {
    // Try to get from pool first
    {
      auto pool = connection_pool_.Lock();
      if (!pool->empty()) {
        auto socket = std::move(pool->back());
        pool->pop_back();
        return std::make_unique<Connection>(std::move(socket), this);
      }
    }
    
    // Create new connection
    auto socket = CreateNewConnection();
    return std::make_unique<Connection>(std::move(socket), this);
  }

private:
  engine::io::Socket CreateNewConnection() {
    auto socket = engine::io::Socket::Create(
      config_.server_addr.Domain(),
      engine::io::SocketType::kStream
    );
    
    if (config_.tcp_nodelay) {
      socket.SetOption(engine::io::Socket::Option::kTcpNoDelay, 1);
    }
    
    auto deadline = engine::Deadline::FromDuration(config_.connection_timeout);
    socket.Connect(config_.server_addr, deadline);
    
    return socket;
  }
  
  void ReturnConnection(engine::io::Socket socket) {
    if (!socket.IsValid()) {
      return;
    }
    
    auto pool = connection_pool_.Lock();
    if (pool->size() < config_.max_connections) {
      pool->push_back(std::move(socket));
    }
    // Otherwise, socket will be closed automatically
  }
  
  Config config_;
  concurrent::Variable<std::vector<engine::io::Socket>> connection_pool_;
};
```

## UDP Client Implementation

### Basic UDP Client
```cpp
class UdpClient {
public:
  explicit UdpClient(const engine::io::Sockaddr& server_addr)
    : server_addr_(server_addr) {}
  
  void Connect() {
    socket_ = engine::io::Socket::Create(
      server_addr_.Domain(),
      engine::io::SocketType::kDgram
    );
  }
  
  std::string SendRequest(const std::string& request) {
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(5));
    
    socket_.SendTo(request.data(), request.size(), server_addr_, deadline);
    
    std::array<char, 65536> buffer;
    engine::io::Sockaddr response_addr;
    auto bytes_received = socket_.RecvFrom(
      buffer.data(), buffer.size(), response_addr, deadline
    );
    
    return std::string(buffer.data(), bytes_received);
  }
  
  void Disconnect() {
    if (socket_.IsValid()) {
      socket_.Close();
    }
  }

private:
  engine::io::Sockaddr server_addr_;
  engine::io::Socket socket_;
};
```

## TLS/SSL Support

### TLS TCP Server
```cpp
class TlsTcpServer {
public:
  struct TlsConfig {
    std::string cert_file;
    std::string key_file;
    std::string ca_file;
    bool verify_client = false;
  };
  
  TlsTcpServer(const engine::io::Sockaddr& listen_addr, TlsConfig tls_config)
    : listen_addr_(listen_addr), tls_config_(std::move(tls_config)) {}
  
  void Start() {
    listen_socket_ = engine::io::Socket::Create(
      listen_addr_.Domain(),
      engine::io::SocketType::kStream
    );
    
    listen_socket_.Bind(listen_addr_);
    listen_socket_.Listen(128);
    
    LOG_INFO() << "TLS TCP server listening on " << listen_addr_.PrimaryAddressString();
    
    while (!should_stop_) {
      try {
        auto client_socket = listen_socket_.Accept({});
        
        utils::Async("tls_client_handler", 
          [this, socket = std::move(client_socket)]() mutable {
            HandleTlsClient(std::move(socket));
          }
        ).Detach();
        
      } catch (const std::exception& e) {
        LOG_ERROR() << "TLS accept failed: " << e.what();
      }
    }
  }

private:
  void HandleTlsClient(engine::io::Socket client_socket) {
    try {
      // Wrap socket with TLS
      auto tls_wrapper = engine::io::TlsWrapper::StartTlsServer(
        std::move(client_socket),
        tls_config_.cert_file,
        tls_config_.key_file,
        tls_config_.ca_file
      );
      
      if (tls_config_.verify_client) {
        tls_wrapper.SetVerifyMode(engine::io::TlsWrapper::VerifyMode::kPeer);
      }
      
      std::array<char, 4096> buffer;
      
      while (tls_wrapper.IsValid()) {
        auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(30));
        
        auto bytes_received = tls_wrapper.RecvSome(
          buffer.data(), buffer.size(), deadline
        );
        
        if (bytes_received == 0) {
          break;
        }
        
        // Echo back
        tls_wrapper.SendAll(buffer.data(), bytes_received, deadline);
      }
      
    } catch (const std::exception& e) {
      LOG_ERROR() << "TLS client handler error: " << e.what();
    }
  }
  
  engine::io::Sockaddr listen_addr_;
  TlsConfig tls_config_;
  engine::io::Socket listen_socket_;
  std::atomic<bool> should_stop_{false};
};
```

## Performance Optimization

### High-Performance Socket Configuration
```cpp
void ConfigureHighPerformanceSocket(engine::io::Socket& socket) {
  // Disable Nagle's algorithm for low latency
  socket.SetOption(engine::io::Socket::Option::kTcpNoDelay, 1);
  
  // Enable keep-alive
  socket.SetOption(engine::io::Socket::Option::kKeepAlive, 1);
  
  // Set larger buffer sizes
  socket.SetOption(engine::io::Socket::Option::kSendBufferSize, 1048576); // 1MB
  socket.SetOption(engine::io::Socket::Option::kReceiveBufferSize, 1048576); // 1MB
  
  // Enable address reuse
  socket.SetOption(engine::io::Socket::Option::kReuseAddr, 1);
}
```

### Vectored I/O Example
```cpp
class VectoredIoExample {
public:
  void SendMultipleBuffers(engine::io::Socket& socket, 
                          const std::vector<std::string>& messages) {
    std::vector<engine::io::IoData> io_data;
    
    for (const auto& msg : messages) {
      io_data.emplace_back(msg.data(), msg.size());
    }
    
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
    socket.SendAll(io_data, deadline);
  }
  
  std::vector<std::string> ReceiveMultipleBuffers(engine::io::Socket& socket,
                                                 size_t num_buffers,
                                                 size_t buffer_size) {
    std::vector<std::vector<char>> buffers(num_buffers);
    std::vector<engine::io::IoData> io_data;
    
    for (auto& buffer : buffers) {
      buffer.resize(buffer_size);
      io_data.emplace_back(buffer.data(), buffer.size());
    }
    
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
    auto bytes_received = socket.RecvSome(io_data, deadline);
    
    std::vector<std::string> results;
    size_t offset = 0;
    
    for (size_t i = 0; i < buffers.size() && offset < bytes_received; ++i) {
      size_t chunk_size = std::min(buffer_size, bytes_received - offset);
      results.emplace_back(buffers[i].data(), chunk_size);
      offset += chunk_size;
    }
    
    return results;
  }
};
```

## Advanced TCP Client with Reconnection

### Resilient TCP Client
```cpp
class ResilientTcpClient {
public:
  struct Config {
    engine::io::Sockaddr server_addr;
    std::chrono::seconds connect_timeout{5};
    std::chrono::seconds request_timeout{10};
    size_t max_retries = 3;
    std::chrono::milliseconds retry_delay{1000};
    bool tcp_nodelay = true;
  };
  
  explicit ResilientTcpClient(Config config) : config_(std::move(config)) {}
  
  std::string SendRequest(const std::string& request) {
    for (size_t attempt = 0; attempt <= config_.max_retries; ++attempt) {
      try {
        EnsureConnected();
        return DoSendRequest(request);
        
      } catch (const std::exception& e) {
        LOG_WARNING() << "Request attempt " << (attempt + 1) << " failed: " << e.what();
        
        Disconnect();
        
        if (attempt < config_.max_retries) {
          engine::SleepFor(config_.retry_delay * (1 << attempt)); // Exponential backoff
        }
      }
    }
    
    throw std::runtime_error("All retry attempts failed");
  }
  
  void Disconnect() {
    if (socket_.IsValid()) {
      socket_.Close();
    }
    connected_ = false;
  }

private:
  void EnsureConnected() {
    if (connected_ && socket_.IsValid()) {
      return;
    }
    
    socket_ = engine::io::Socket::Create(
      config_.server_addr.Domain(),
      engine::io::SocketType::kStream
    );
    
    ConfigureHighPerformanceSocket(socket_);
    
    auto deadline = engine::Deadline::FromDuration(config_.connect_timeout);
    socket_.Connect(config_.server_addr, deadline);
    connected_ = true;
  }
  
  std::string DoSendRequest(const std::string& request) {
    auto deadline = engine::Deadline::FromDuration(config_.request_timeout);
    socket_.SendAll(request.data(), request.size(), deadline);
    
    std::array<char, 4096> buffer;
    auto bytes_received = socket_.RecvSome(buffer.data(), buffer.size(), deadline);
    return std::string(buffer.data(), bytes_received);
  }
  
  void Connect() {
    socket_ = engine::io::Socket::Create(
      server_addr_.Domain(),
      engine::io::SocketType::kStream
    );
    
    ConfigureHighPerformanceSocket(socket_);
    
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(5));
    socket_.Connect(server_addr_, deadline);
  }
  
  std::string SendRequest(const std::string& request) {
    auto deadline = engine::Deadline::FromDuration(std::chrono::seconds(10));
    socket_.SendAll(request.data(), request.size(), deadline);
    
    std::array<char, 4096> buffer;
    auto bytes_received = socket_.RecvSome(buffer.data(), buffer.size(), deadline);
    return std::string(buffer.data(), bytes_received);
  }
  
  void Disconnect() {
    if (socket_.IsValid()) {
      socket_.Close();
    }
  }
  
  engine::io::Sockaddr server_addr_;
  engine::io::Socket socket_;
};
```

## Monitoring and Metrics

### Network Metrics Collection
```cpp
class NetworkMetrics {
public:
  void RecordConnection(const std::string& protocol, bool success) {
    auto& counter = utils::statistics::GetMetric("network.connections.total");
    counter.Inc({{"protocol", protocol}, {"status", success ? "success" : "failure"}});
  }
  
  void RecordBytesTransferred(const std::string& protocol, 
                             const std::string& direction, 
                             size_t bytes) {
    auto& counter = utils::statistics::GetMetric("network.bytes.total");
    counter.Add(bytes, {{"protocol", protocol}, {"direction", direction}});
  }
  
  void RecordLatency(const std::string& operation,
                    std::chrono::milliseconds latency) {
    auto& histogram = utils::statistics::GetMetric("network.operation.duration");
    histogram.Account(latency.count(), {{"operation", operation}});
  }
};
```

### Connection Health Monitoring
```cpp
class ConnectionHealthMonitor {
public:
  struct ConnectionStats {
    std::chrono::steady_clock::time_point created_at;
    std::chrono::steady_clock::time_point last_activity;
    size_t bytes_sent = 0;
    size_t bytes_received = 0;
    size_t errors = 0;
  };
  
  void RegisterConnection(const std::string& connection_id) {
    auto stats = stats_.Lock();
    (*stats)[connection_id] = ConnectionStats{
      .created_at = std::chrono::steady_clock::now(),
      .last_activity = std::chrono::steady_clock::now()
    };
  }
  
  void UpdateActivity(const std::string& connection_id,
                     size_t bytes_sent, size_t bytes_received) {
    auto stats = stats_.Lock();
    auto it = stats->find(connection_id);
    if (it != stats->end()) {
      it->second.last_activity = std::chrono::steady_clock::now();
      it->second.bytes_sent += bytes_sent;
      it->second.bytes_received += bytes_received;
    }
  }
  
  void RecordError(const std::string& connection_id) {
    auto stats = stats_.Lock();
    auto it = stats->find(connection_id);
    if (it != stats->end()) {
      it->second.errors++;
    }
  }
  
  std::vector<std::string> GetIdleConnections(std::chrono::seconds idle_threshold) {
    auto stats = stats_.Lock();
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> idle_connections;
    
    for (const auto& [id, stat] : *stats) {
      if (now - stat.last_activity > idle_threshold) {
        idle_connections.push_back(id);
      }
    }
    
    return idle_connections;
  }

private:
  concurrent::Variable<std::unordered_map<std::string, ConnectionStats>> stats_;
};
```

## Testing Patterns

### TCP Server Testing
```cpp
#include <userver/utest/utest.hpp>

class TcpServerTest : public ::testing::Test {
protected:
  void SetUp() override {
    server_addr_ = engine::io::Sockaddr::MakeLoopbackAddress(0); // Random port
    server_ = std::make_unique<TcpServer>(server_addr_);
    
    // Start server in background
    server_task_ = utils::Async("test_server", [this]() {
      server_->Start();
    });
    
    // Wait for server to start
    engine::SleepFor(std::chrono::milliseconds(100));
  }
  
  void TearDown() override {
    server_->Stop();
    if (server_task_.IsValid()) {
      server_task_.SyncCancel();
    }
  }
  
  engine::io::Sockaddr server_addr_;
  std::unique_ptr<TcpServer> server_;
  engine::TaskWithResult<void> server_task_;
};

UTEST_F(TcpServerTest, EchoMessage) {
  TcpClient client(server_addr_);
  client.Connect();
  
  std::string test_message = "Hello, TCP Server!";
  auto response = client.SendRequest(test_message);
  
  EXPECT_EQ(response, test_message);
  
  client.Disconnect();
}

UTEST_F(TcpServerTest, MultipleClients) {
  constexpr int num_clients = 10;
  std::vector<std::unique_ptr<TcpClient>> clients;
  
  // Create and connect clients
  for (int i = 0; i < num_clients; ++i) {
    auto client = std::make_unique<TcpClient>(server_addr_);
    client->Connect();
    clients.push_back(std::move(client));
  }
  
  // Send messages concurrently
  std::vector<engine::TaskWithResult<std::string>> tasks;
  for (int i = 0; i < num_clients; ++i) {
    tasks.push_back(utils::Async("client_task", [&clients, i]() {
      std::string message = "Message from client " + std::to_string(i);
      return clients[i]->SendRequest(message);
    }));
  }
  
  // Verify responses
  for (int i = 0; i < num_clients; ++i) {
    std::string expected = "Message from client " + std::to_string(i);
    EXPECT_EQ(tasks[i].Get(), expected);
  }
}
```

### UDP Testing
```cpp
UTEST(UdpTest, SendReceive) {
  auto server_addr = engine::io::Sockaddr::MakeLoopbackAddress(0);
  UdpServer server(server_addr);
  
  auto server_task = utils::Async("udp_server", [&server]() {
    server.Start();
  });
  
  engine::SleepFor(std::chrono::milliseconds(100));
  
  UdpClient client(server_addr);
  client.Connect();
  
  std::string test_message = "UDP Test Message";
  auto response = client.SendRequest(test_message);
  
  EXPECT_EQ(response, "Echo: " + test_message);
  
  server.Stop();
  server_task.SyncCancel();
}
```

## Best Practices

### Socket Management
- Always check socket validity before operations
- Set appropriate timeouts for all I/O operations
- Use proper socket options for performance
- Handle connection errors gracefully
- Implement connection pooling for clients

### Performance Guidelines
- Use vectored I/O for multiple buffers
- Configure socket buffer sizes appropriately
- Disable Nagle's algorithm for low-latency applications
- Use connection pooling to reduce overhead
- Monitor and tune network parameters

### Error Handling
- Distinguish between recoverable and non-recoverable errors
- Implement retry logic with exponential backoff
- Log network errors with sufficient context
- Handle partial reads/writes correctly
- Use timeouts to prevent hanging operations

### Security Considerations
- Validate all incoming data
- Use TLS for sensitive communications
- Implement rate limiting to prevent abuse
- Monitor for suspicious network activity
- Use secure socket options

### Resource Management
- Close sockets properly to avoid resource leaks
- Monitor connection counts and limits
- Implement connection timeouts
- Use RAII for automatic resource cleanup
- Monitor memory usage for large-scale applications

## Configuration Examples

### TCP Server Configuration
```yaml
components_manager:
  components:
    tcp-server:
      port: 8080
      max-connections: 1000
      connection-timeout: 60s
      buffer-size: 65536
      keep-alive: true
      tcp-nodelay: true
```

### UDP Server Configuration
```yaml
components_manager:
  components:
    udp-server:
      port: 8081
      max-packet-size: 65536
      receive-timeout: 1s
      send-timeout: 5s
      buffer-size: 1048576
```

### TLS Configuration
```yaml
components_manager:
  components:
    tls-server:
      port: 8443
      cert-file: /path/to/server.crt
      key-file: /path/to/server.key
      ca-file: /path/to/ca.crt
      verify-client: true
      cipher-suites: "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
```

## Cross-References

- **Memory Bank**: [`networking-patterns.md`](../../memory-bank/specialized/tcp-udp-networking/networking-patterns.md) - Advanced networking patterns
- **Memory Bank**: [`framework-core.md`](../../memory-bank/main/framework-core.md) - Core framework patterns
- **Memory Bank**: [`async-programming.md`](../../memory-bank/main/async-programming.md) - Asynchronous patterns
- **Rules**: [`http-https.md`](./http-https.md) - HTTP/HTTPS patterns
- **Rules**: [`grpc.md`](./grpc.md) - gRPC patterns
- **Rules**: [`network-security.md`](./network-security.md) - Network security patterns
- **Rules**: [`error-handling.md`](../10-development/error-handling.md) - Error handling patterns
- **Rules**: [`testing.md`](../10-development/testing.md) - Testing strategies