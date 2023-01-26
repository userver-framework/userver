#include "listener_impl.hpp"

#include <netinet/tcp.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

#include <server/net/create_socket.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

ListenerImpl::ListenerImpl(engine::TaskProcessor& task_processor,
                           std::shared_ptr<EndpointInfo> endpoint_info,
                           request::ResponseDataAccounter& data_accounter)
    : task_processor_(task_processor),
      endpoint_info_(std::move(endpoint_info)),
      stats_(std::make_shared<Stats>()),
      data_accounter_(data_accounter),
      socket_listener_task_(engine::CriticalAsyncNoSpan(
          task_processor_,
          [this](engine::io::Socket&& request_socket) {
            while (!engine::current_task::ShouldCancel()) {
              try {
                AcceptConnection(request_socket);
              } catch (const engine::io::IoCancelled&) {
                break;
              } catch (const std::exception& ex) {
                LOG_ERROR() << "can't accept connection: " << ex;

                // If we're out of files, allow other coroutines to close old
                // connections
                engine::Yield();
              }
            }
          },
          CreateSocket(endpoint_info_->listener_config))) {}

ListenerImpl::~ListenerImpl() {
  LOG_TRACE() << "Stopping socket listener task";
  socket_listener_task_.SyncCancel();
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
    LOG_LIMITED_WARNING() << endpoint_info_->GetDescription()
                          << " reached max_connections="
                          << endpoint_info_->listener_config.max_connections
                          << ", dropping connection #" << new_connection_count;
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

  if (peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet6 ||
      peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet)
    peer_socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);

  LOG_TRACE() << "Creating connection for fd " << fd;

  auto connection_ptr = Connection::Create(
      task_processor_, endpoint_info_->listener_config.connection_config,
      endpoint_info_->listener_config.handler_defaults, std::move(peer_socket),
      endpoint_info_->request_handler, stats_, data_accounter_);
  connection_ptr->SetCloseCb([endpoint_info = endpoint_info_]() {
    --endpoint_info->connection_count;
  });

  AddConnection(connection_ptr);

  LOG_TRACE() << "Starting connection for fd " << fd;
  connection_ptr->Start();
  LOG_TRACE() << "Started connection for fd " << fd;
}

void ListenerImpl::AddConnection(
    const std::shared_ptr<Connection>& connection) {
  int fd = connection->Fd();
  UASSERT(fd >= 0);

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

}  // namespace server::net

USERVER_NAMESPACE_END
