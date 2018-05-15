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

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <utils/check_syscall.hpp>

namespace server {
namespace net {

namespace {

static const size_t kConnectionsToCloseQueueCapacity = 64;

int CreateSocket(uint16_t port, int backlog) {
  int fd = utils::CheckSyscall(socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0),
                               "creating socket");

  const int reuse = 1;
  utils::CheckSyscall(
      setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)),
      "setting SO_REUSEPORT");

  sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(port);
  addr.sin6_addr = in6addr_any;
  utils::CheckSyscall(
      bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)),
      "binding a socket");
  utils::CheckSyscall(listen(fd, backlog), "listening on a socket");
  return fd;
}

}  // namespace

ListenerImpl::ListenerImpl(engine::ev::ThreadControl& thread_control,
                           engine::TaskProcessor& task_processor,
                           std::shared_ptr<EndpointInfo> endpoint_info)
    : engine::ev::ThreadControl(thread_control),
      task_processor_(task_processor),
      endpoint_info_(std::move(endpoint_info)),
      is_closing_(false),
      connections_to_close_(kConnectionsToCloseQueueCapacity),
      close_connections_task_(task_processor_, [this] { CloseConnections(); }),
      past_processed_requests_count_(0),
      pending_setup_connection_count_(0),
      pending_close_connection_count_(0) {
  int request_fd = CreateSocket(endpoint_info_->listener_config.port,
                                endpoint_info_->listener_config.backlog);
  int monitor_fd = CreateSocket(endpoint_info_->listener_config.monitor_port,
                                endpoint_info_->listener_config.backlog);

  request_socket_listener_ = std::make_unique<engine::SocketListener>(
      *this, task_processor_, request_fd,
      engine::SocketListener::ListenMode::kRead,
      [this](int fd) {
        try {
          AcceptConnection(fd, Connection::Type::kRequest);
        } catch (const std::exception& ex) {
          LOG_ERROR() << "can't accept connection: " << ex.what();
        }
        return engine::SocketListener::Result::kOk;
      },
      nullptr);
  monitor_socket_listener_ = std::make_unique<engine::SocketListener>(
      *this, task_processor_, monitor_fd,
      engine::SocketListener::ListenMode::kRead,
      [this](int fd) {
        AcceptConnection(fd, Connection::Type::kMonitor);
        return engine::SocketListener::Result::kOk;
      },
      nullptr);
}

ListenerImpl::~ListenerImpl() {
  LOG_TRACE() << "Stopping connection close task";
  close_connections_task_.Stop();
  LOG_TRACE() << "Stopped connection close task";

  LOG_TRACE() << "Closing request fd " << request_socket_listener_->Fd();
  request_socket_listener_->Stop();

  while (pending_setup_connection_count_ > 0 ||
         pending_close_connection_count_ > 0)
    engine::Sleep(std::chrono::milliseconds(10));

  close(request_socket_listener_->Fd());
  LOG_TRACE() << "Closed request fd " << request_socket_listener_->Fd();
  LOG_TRACE() << "Closing monitor fd " << monitor_socket_listener_->Fd();
  monitor_socket_listener_->Stop();
  close(monitor_socket_listener_->Fd());
  LOG_TRACE() << "Closed monitor fd " << monitor_socket_listener_->Fd();

  std::lock_guard<std::mutex> lock(connections_mutex_);
  for (auto& item : connections_) {
    int fd = item.first;
    auto& connection_ptr = item.second;
    if (connection_ptr->GetType() != Connection::Type::kMonitor) {
      past_processed_requests_count_ += connection_ptr->ProcessedRequestCount();
    }

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
  stats.total_processed_requests = past_processed_requests_count_;
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (const auto& item : connections_) {
      const auto& connection_ptr = item.second;
      if (connection_ptr->GetType() == Connection::Type::kMonitor) {
        continue;
      }
      ++stats.total_connections;
      stats.total_processed_requests += connection_ptr->ProcessedRequestCount();
      stats.total_active_requests += connection_ptr->ActiveRequestCount();
      stats.total_parsing_requests += connection_ptr->ParsingRequestCount();
      stats.conn_active_requests.push_back(
          connection_ptr->ActiveRequestCount());
      stats.conn_pending_responses.push_back(
          connection_ptr->PendingResponseCount());
    }
  }
  stats.max_listener_connections = stats.total_connections;
  stats.listener_connections.push_back(stats.total_connections);
  return stats;
}

engine::TaskProcessor& ListenerImpl::GetTaskProcessor() const {
  return task_processor_;
}

void ListenerImpl::AcceptConnection(int listen_fd, Connection::Type type) {
  sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  socklen_t addrlen = sizeof(addr);
  int fd =
      utils::CheckSyscall(accept4(listen_fd, reinterpret_cast<sockaddr*>(&addr),
                                  &addrlen, SOCK_NONBLOCK),
                          "accepting connection");

  auto new_connection_count = ++endpoint_info_->connection_count;
  if (new_connection_count > endpoint_info_->listener_config.max_connections) {
    LOG_WARNING() << "Reached connection limit, dropping connection #"
                  << new_connection_count << '/'
                  << endpoint_info_->listener_config.max_connections;
    utils::CheckSyscall(close(fd), "closing connection");
    --endpoint_info_->connection_count;
    return;
  }
  LOG_DEBUG() << "Accepted connection #" << new_connection_count << '/'
              << endpoint_info_->listener_config.max_connections;
  ++pending_setup_connection_count_;

  engine::Async([this, fd, type, addr]() {
    try {
      SetupConnection(fd, type, addr);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "can't setup connection: " << ex.what();
    }
    --pending_setup_connection_count_;
  });
}

void ListenerImpl::SetupConnection(int fd, Connection::Type type,
                                   const sockaddr_in6& addr) {
  const int nodelay = 1;
  utils::CheckSyscall(
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)),
      "setting TCPNODELAY");

  LOG_TRACE() << "Creating connection for fd " << fd;
  auto connection_ptr = std::make_unique<Connection>(
      *this, fd, endpoint_info_->listener_config.connection_config, type,
      endpoint_info_->request_handler, addr,
      [this](int fd) { EnqueueConnectionClose(fd); });
  LOG_TRACE() << "Registering connection for fd " << fd;
  auto connection_handle = [this, fd, &connection_ptr] {
    std::lock_guard<std::mutex> lock(connections_mutex_);
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
      std::lock_guard<std::mutex> lock(connections_mutex_);
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
    if (connection_ptr->GetType() != Connection::Type::kMonitor) {
      past_processed_requests_count_ += connection_ptr->ProcessedRequestCount();
    }

    ++pending_close_connection_count_;
    engine::Async(
        [this, fd](std::unique_ptr<Connection>&& connection_ptr) {
          LOG_TRACE() << "Destroying connection for fd " << fd;
          connection_ptr.reset();
          LOG_TRACE() << "Destroyed connection for fd " << fd;
          --endpoint_info_->connection_count;
          --pending_close_connection_count_;
        },
        std::move(connection_ptr));
  }
}

}  // namespace net
}  // namespace server
