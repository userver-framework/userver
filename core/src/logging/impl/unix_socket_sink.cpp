#include "unix_socket_sink.hpp"

#include <fmt/format.h>

#include <userver/engine/io/sockaddr.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

void UnixSocketClient::connect(std::string_view filename) {
    const auto addr = engine::io::Sockaddr::MakeUnixSocketAddress(filename);
    socket_ = engine::io::Socket{engine::io::AddrDomain::kUnix, engine::io::SocketType::kStream};
    socket_.Connect(addr, {});
}

void UnixSocketClient::send(std::span<const struct iovec> logs) {
    if (logs.empty()) {
        return;
    }

    const auto send_result = socket_.SendAll(logs.data(), logs.size(), {});
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

void UnixSocketClient::close() { socket_.Close(); }

UnixSocketClient::~UnixSocketClient() { socket_.Close(); }

void UnixSocketSink::Write(std::span<const struct iovec> logs) { client_.send(logs); }

void UnixSocketSink::Close() { client_.close(); }

}  // namespace logging::impl

USERVER_NAMESPACE_END
