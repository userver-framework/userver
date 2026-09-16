#pragma once

#include <mutex>
#include <string_view>
#include <vector>

#include <logging/impl/base_sink.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class TcpSocketClient final {
public:
    explicit TcpSocketClient(std::vector<engine::io::Sockaddr> addrs);

    TcpSocketClient(const TcpSocketClient&) = delete;
    TcpSocketClient(TcpSocketClient&&) = default;
    TcpSocketClient& operator=(const TcpSocketClient&) = delete;
    TcpSocketClient& operator=(TcpSocketClient&&) = default;
    ~TcpSocketClient();

    void Connect();
    void Send(std::span<const struct iovec> logs);
    bool IsConnected();
    void Close();

private:
    std::vector<engine::io::Sockaddr> addrs_;
    engine::io::Socket socket_;
};

class TcpSocketSink final : public BaseSink {
public:
    explicit TcpSocketSink(std::vector<engine::io::Sockaddr> addr);

    TcpSocketSink() = delete;
    ~TcpSocketSink() override = default;

    void Close();

    void Write(std::span<const struct iovec> logs) final;

private:
    std::mutex mutex_;
    impl::TcpSocketClient client_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
