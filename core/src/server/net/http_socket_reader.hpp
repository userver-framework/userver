#pragma once

#include <memory>
#include <string>

#include <userver/engine/io/socket.hpp>

#include <server/net/connection_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

class SocketBufferedReader final {
public:
    SocketBufferedReader(const ConnectionConfig& config, std::string peer_name);
    SocketBufferedReader(SocketBufferedReader&&) = delete;
    SocketBufferedReader& operator=(const SocketBufferedReader&) = delete;
    SocketBufferedReader& operator=(SocketBufferedReader&&) = delete;
    ~SocketBufferedReader() = default;

    bool TryRead(engine::io::RwBase& peer_socket, int fd);
    std::size_t DoRead(engine::io::RwBase& peer_socket, engine::Deadline deadline, int fd);

    bool IsFull() const noexcept;
    bool IsEmpty() const noexcept;
    std::string_view PeekAsStringView() const noexcept;
    void ResetView() noexcept;
    std::size_t BufferSize() const noexcept;

    const std::string& GetPeerName() const noexcept;

private:
    const std::chrono::seconds keepalive_timeout_{};
    const std::string peer_name_;
    std::string pending_data_;
    std::size_t pending_data_size_{0};
};

}  // namespace server::net

USERVER_NAMESPACE_END
