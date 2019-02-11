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

#include <blocking/fs/read.hpp>
#include <blocking/fs/write.hpp>
#include <engine/async.hpp>
#include <engine/io/error.hpp>
#include <engine/io/socket.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>

namespace server {
namespace net {

namespace {
engine::io::Socket CreateUnixSocket(const std::string& path, int backlog) {
  engine::io::AddrStorage addr_storage;
  auto* sa = addr_storage.As<struct sockaddr_un>();
  sa->sun_family = AF_UNIX;

  if (path.size() >= sizeof(sa->sun_path))
    throw std::runtime_error("unix socket path is too long (" + path + ")");
  if (path.empty())
    throw std::runtime_error(
        "unix socket path is empty, abstract sockets are not supported");
  if (path[0] != '/')
    throw std::runtime_error("unix socket path must be absolute (" + path +
                             ")");
  std::strncpy(sa->sun_path, path.c_str(), sizeof(sa->sun_path));

  /* Use blocking API here, it is not critical as CreateUnixSocket() is called
   * on startup only */

  if (blocking::fs::GetFileType(path) ==
      boost::filesystem::file_type::socket_file)
    blocking::fs::RemoveSingleFile(path);

  auto socket = engine::io::Listen(
      engine::io::Addr(addr_storage, SOCK_STREAM, 0), backlog);

  auto perms = static_cast<boost::filesystem::perms>(0666);
  blocking::fs::Chmod(path, perms);
  return socket;
}

engine::io::Socket CreateIpv6Socket(uint16_t port, int backlog) {
  engine::io::AddrStorage addr_storage;
  auto* sa = addr_storage.As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_port = htons(port);
  sa->sin6_addr = in6addr_any;

  return engine::io::Listen(engine::io::Addr(addr_storage, SOCK_STREAM, 0),
                            backlog);
}

engine::io::Socket CreateSocket(const ListenerConfig& config) {
  if (config.unix_socket_path.empty())
    return CreateIpv6Socket(config.port, config.backlog);
  else
    return CreateUnixSocket(config.unix_socket_path, config.backlog);
}

}  // namespace

ListenerImpl::ListenerImpl(engine::TaskProcessor& task_processor,
                           std::shared_ptr<EndpointInfo> endpoint_info)
    : task_processor_(task_processor),
      endpoint_info_(std::move(endpoint_info)),
      stats_(std::make_shared<Stats>()),
      socket_listener_task_(engine::CriticalAsync(
          task_processor_,
          [this](engine::io::Socket&& request_socket) {
            while (!engine::current_task::ShouldCancel()) {
              try {
                AcceptConnection(request_socket);
              } catch (const engine::io::IoCancelled&) {
                break;
              } catch (const std::exception& ex) {
                LOG_ERROR() << "can't accept connection: " << ex.what();
              }
            }
          },
          CreateSocket(endpoint_info_->listener_config))) {}

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

  if (peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet6 ||
      peer_socket.Getsockname().Domain() == engine::io::AddrDomain::kInet)
    peer_socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);

  LOG_TRACE() << "Creating connection for fd " << fd;
  auto connection_ptr = std::make_shared<Connection>(
      task_processor_, endpoint_info_->listener_config.connection_config,
      std::move(peer_socket), endpoint_info_->request_handler, stats_);
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
