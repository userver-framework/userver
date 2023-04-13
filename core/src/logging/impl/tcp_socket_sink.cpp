#include "tcp_socket_sink.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

TcpSocketClient::TcpSocketClient(std::vector<engine::io::Sockaddr> addrs)
    : addrs_(std::move(addrs)) {}

void TcpSocketClient::Connect() {
  Close();
  for (const auto& addr : addrs_) {
    socket_ =
        engine::io::Socket{addr.Domain(), engine::io::SocketType::kStream};
    try {
      socket_.Connect(addr, {});
      break;
    } catch (std::exception&) {
      continue;
    }
  }
  if (socket_.Fd() == -1) {
    std::string list{};
    for (const auto& addr : addrs_) {
      list += addr.PrimaryAddressString() + ", ";
    }
    throw std::runtime_error(
        fmt::format("TcpSocketSink failed to connect to {}", list));
  }
  socket_.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
}

void TcpSocketClient::Send(const char* data, size_t n_bytes) {
  auto send_result = socket_.SendAll(data, n_bytes, {});
  if (n_bytes != send_result) {
    throw std::runtime_error(fmt::format(
        "Failed to send {} bytes because the remote closed the connection",
        n_bytes));
  }
}

void TcpSocketClient::Close() { socket_.Close(); }

bool TcpSocketClient::IsConnected() { return socket_.Fd() != -1; }

TcpSocketClient::~TcpSocketClient() { socket_.Close(); }

TcpSocketSink::TcpSocketSink(std::vector<engine::io::Sockaddr> addr)
    : client_{std::move(addr)} {}

void TcpSocketSink::Close() {
  std::lock_guard lock{GetMutex()};
  client_.Close();
}

void TcpSocketSink::Write(std::string_view log) {
  if (!client_.IsConnected()) {
    client_.Connect();
  }
  client_.Send(log.data(), log.size());
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
