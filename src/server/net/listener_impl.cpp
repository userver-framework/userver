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
      stats_(std::make_shared<Stats>()),
      socket_listener_task_(engine::CriticalAsync(
          task_processor_,
          [this](engine::io::Socket&& request_socket) {
            while (true) {
              try {
                AcceptConnection(request_socket);
              } catch (const std::exception& ex) {
                LOG_ERROR() << "can't accept connection: " << ex.what();
              }
            }
          },
          CreateSocket(endpoint_info_->listener_config.port,
                       endpoint_info_->listener_config.backlog))) {}

ListenerImpl::~ListenerImpl() {
  LOG_TRACE() << "Stopping socket listener task";
  socket_listener_task_ = {};
  LOG_TRACE() << "Stopped socket listener task";

  CloseConnections();
}

Stats ListenerImpl::GetStats() const { return *stats_; }

engine::TaskProcessor& ListenerImpl::GetTaskProcessor() const {
  return task_processor_;
}

void ListenerImpl::AcceptConnection(engine::io::Socket& request_socket) {
  auto peer_socket = request_socket.Accept({});

  auto new_connection_count = ++endpoint_info_->connection_count;
  if (new_connection_count > endpoint_info_->listener_config.max_connections) {
    LOG_WARNING() << "Reached connection limit, dropping connection #"
                  << new_connection_count << '/'
                  << endpoint_info_->listener_config.max_connections;
    peer_socket.Close();
    --endpoint_info_->connection_count;
    return;
  }

  LOG_DEBUG() << "Accepted connection #" << new_connection_count << '/'
              << endpoint_info_->listener_config.max_connections;
  SetupConnection(std::move(peer_socket));
}

void ListenerImpl::SetupConnection(engine::io::Socket peer_socket) {
  const auto fd = peer_socket.Fd();

  peer_socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);

  LOG_TRACE() << "Creating connection for fd " << fd;
  auto connection_ptr = std::make_shared<Connection>(
      task_processor_, endpoint_info_->listener_config.connection_config,
      std::move(peer_socket), endpoint_info_->request_handler, stats_);

  AddConnection(connection_ptr);

  LOG_TRACE() << "Starting connection for fd " << fd;
  connection_ptr->Start();
  LOG_TRACE() << "Started connection for fd " << fd;
}

void ListenerImpl::AddConnection(
    const std::shared_ptr<Connection>& connection) {
  int fd = connection->Fd();
  assert(fd >= 0);

  /* connections_ is expected not to grow too big:
   * it is limited to ~max simultaneous connections by all listeners
   */
  if (connections_.size() <= static_cast<unsigned int>(fd))
    connections_.resize(std::max(fd + 1, fd * 2));

  connections_[fd] = connection;
}

void ListenerImpl::CloseConnections() {
  for (auto& weak_ptr : connections_) {
    auto connection = weak_ptr.lock();
    if (connection) connection->Stop();
  }
}

}  // namespace net
}  // namespace server
