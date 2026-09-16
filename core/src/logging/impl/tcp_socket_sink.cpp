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
    : addrs_(std::move(addrs))
{}

void TcpSocketClient::Connect() {
    Close();
    for (const auto& addr : addrs_) {
        socket_ = engine::io::Socket{addr.Domain(), engine::io::SocketType::kStream};
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
        throw std::runtime_error(fmt::format("TcpSocketSink failed to connect to {}", list));
    }
    socket_.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
}

void TcpSocketClient::Send(std::span<const struct iovec> logs) {
    auto send_result = socket_.SendAll(logs.data(), logs.size(), {});
    std::size_t n_bytes = 0;
    for (const auto& log : logs) {
        n_bytes += log.iov_len;
    }
    if (n_bytes != send_result) {
        throw std::runtime_error(
            fmt::format("Failed to send {} bytes because the remote closed the connection", n_bytes)
        );
    }
}

void TcpSocketClient::Close() { socket_.Close(); }

bool TcpSocketClient::IsConnected() { return socket_.Fd() != -1; }

TcpSocketClient::~TcpSocketClient() { socket_.Close(); }

TcpSocketSink::TcpSocketSink(std::vector<engine::io::Sockaddr> addr)
    : client_{std::move(addr)}
{}

void TcpSocketSink::Close() {
    const std::lock_guard lock{mutex_};
    client_.Close();
}

void TcpSocketSink::Write(std::span<const struct iovec> logs) {
    const std::lock_guard lock{mutex_};
    if (!client_.IsConnected()) {
        client_.Connect();
    }
    client_.Send(logs);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
