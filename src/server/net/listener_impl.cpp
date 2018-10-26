#include "listener_impl.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include <engine/async.hpp>
#include <engine/io/socket.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>

namespace server {
namespace net {

namespace {

static const size_t kConnectionsToCloseQueueCapacity = 64;

}  // namespace

engine::io::Socket CreateSocket(uint16_t port, int backlog) {
  engine::io::AddrStorage addr_storage;
  auto* sa = addr_storage.As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_port = htons(port);
  sa->sin6_addr = in6addr_any;

  return engine::io::Listen(engine::io::Addr(addr_storage, SOCK_STREAM, 0),
                            backlog);
}

ListenerImpl::ListenerImpl(engine::TaskProcessor& task_processor,
                           std::shared_ptr<EndpointInfo> endpoint_info)
    : task_processor_(task_processor),
      endpoint_info_(std::move(endpoint_info)),
      is_closing_(false),
      connections_to_close_(kConnectionsToCloseQueueCapacity),
      close_connections_task_(task_processor_, [this] { CloseConnections(); }),
      past_processed_requests_count_(0),
      opened_connection_count_(0),
      closed_connection_count_(0),
      pending_close_connection_count_(0),
      socket_listener_task_(engine::CriticalAsync(
          task_processor_,
          [this](engine::io::Socket&& request_socket) {
            while (true) {
              try {
                AcceptConnection(request_socket,
                                 endpoint_info_->connection_type);
              } catch (const std::exception& ex) {
                LOG_ERROR() << "can't accept connection: " << ex.what();
              }
            }
          },
          CreateSocket(endpoint_info_->listener_config.port,
                       endpoint_info_->listener_config.backlog))) {}

ListenerImpl::~ListenerImpl() {
  LOG_TRACE() << "Stopping connection close task";
  close_connections_task_.Stop();
  LOG_TRACE() << "Stopped connection close task";

  LOG_TRACE() << "Stopping socket listener task";
  socket_listener_task_ = {};
  LOG_TRACE() << "Stopped socket listener task";

  LOG_TRACE() << "Waiting for pending connections closures";
  while (pending_close_connection_count_ > 0)
    engine::SleepFor(std::chrono::milliseconds(10));

  std::lock_guard<engine::Mutex> lock(connections_mutex_);
  for (auto& item : connections_) {
    int fd = item.first;
    auto& connection_ptr = item.second;

    LOG_TRACE() << "Destroying connection for fd " << fd;
    connection_ptr.reset();
    --endpoint_info_->connection_count;
    LOG_TRACE() << "Destroyed connection for fd " << fd;
  }

  LOG_DEBUG() << "Listener stopped, processed "
              << past_processed_requests_count_ << " requests";
}

Stats ListenerImpl::GetStats() const {
  Stats stats;
  size_t active_connections = 0;
  size_t total_processed_requests = past_processed_requests_count_;
  {
    std::lock_guard<engine::Mutex> lock(connections_mutex_);
    for (const auto& item : connections_) {
      const auto& connection_ptr = item.second;
      ++active_connections;
      total_processed_requests += connection_ptr->ProcessedRequestCount();
      stats.active_requests.Add(connection_ptr->ActiveRequestCount());
      stats.parsing_requests.Add(connection_ptr->ParsingRequestCount());
      stats.pending_responses.Add(connection_ptr->PendingResponseCount());
      stats.conn_processed_requests.Add(
          connection_ptr->ProcessedRequestCount());
    }
  }
  stats.active_connections.Add(active_connections);
  stats.listener_processed_requests.Add(total_processed_requests);
  // NOTE: Order ensures closed <= opened
  stats.total_closed_connections.Add(closed_connection_count_);
  stats.total_opened_connections.Add(opened_connection_count_);
  return stats;
}

engine::TaskProcessor& ListenerImpl::GetTaskProcessor() const {
  return task_processor_;
}

void ListenerImpl::AcceptConnection(engine::io::Socket& request_socket,
                                    Connection::Type type) {
  auto peer_socket = request_socket.Accept({});

  auto new_connection_count = ++endpoint_info_->connection_count;
  ++opened_connection_count_;
  if (new_connection_count > endpoint_info_->listener_config.max_connections) {
    LOG_WARNING() << "Reached connection limit, dropping connection #"
                  << new_connection_count << '/'
                  << endpoint_info_->listener_config.max_connections;
    peer_socket.Close();
    --endpoint_info_->connection_count;
    ++closed_connection_count_;
    return;
  }

  LOG_DEBUG() << "Accepted connection #" << new_connection_count << '/'
              << endpoint_info_->listener_config.max_connections;
  SetupConnection(std::move(peer_socket), type);
}

void ListenerImpl::SetupConnection(engine::io::Socket peer_socket,
                                   Connection::Type type) {
  const auto fd = peer_socket.Fd();

  peer_socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);

  LOG_TRACE() << "Creating connection for fd " << fd;
  auto connection_ptr = std::make_unique<Connection>(
      task_processor_, endpoint_info_->listener_config.connection_config,
      std::move(peer_socket), type, endpoint_info_->request_handler,
      [this](int fd) { EnqueueConnectionClose(fd); });
  LOG_TRACE() << "Registering connection for fd " << fd;
  auto connection_handle = [this, fd, &connection_ptr] {
    std::lock_guard<engine::Mutex> lock(connections_mutex_);
    auto insertion_result = connections_.emplace(fd, std::move(connection_ptr));
    assert(insertion_result.second);
    return insertion_result.first->second.get();
  }();
  LOG_TRACE() << "Starting connection for fd " << fd;
  connection_handle->Start();
  LOG_TRACE() << "Started connection for fd " << fd;
}

void ListenerImpl::EnqueueConnectionClose(int fd) {
  connections_to_close_.push(fd);
  close_connections_task_.Notify();
}

void ListenerImpl::CloseConnections() {
  int fd = -1;
  while (connections_to_close_.pop(fd)) {
    std::unique_ptr<Connection> connection_ptr;
    {
      std::lock_guard<engine::Mutex> lock(connections_mutex_);
      auto it = connections_.find(fd);
      if (it != connections_.end()) {
        connection_ptr = std::move(it->second);
        connections_.erase(it);
      }
    }
    if (!connection_ptr) {
      throw std::logic_error("Attempt to close unregistered fd " +
                             std::to_string(fd));
    }
    if (connection_ptr->GetType() != Connection::Type::kMonitor)
      past_processed_requests_count_ += connection_ptr->ProcessedRequestCount();

    ++pending_close_connection_count_;
    engine::CriticalAsync(
        [this, fd](std::unique_ptr<Connection>&& connection_ptr) {
          auto type = connection_ptr->GetType();
          LOG_TRACE() << "Destroying connection for fd " << fd;
          connection_ptr.reset();
          LOG_TRACE() << "Destroyed connection for fd " << fd;
          --endpoint_info_->connection_count;
          if (type != Connection::Type::kMonitor) ++closed_connection_count_;
          --pending_close_connection_count_;
        },
        std::move(connection_ptr))
        .Detach();
  }
}

}  // namespace net
}  // namespace server
