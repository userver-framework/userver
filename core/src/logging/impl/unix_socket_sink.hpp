#pragma once

#include <span>
#include <string>

#include <logging/impl/base_sink.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class UnixSocketClient final {
public:
    UnixSocketClient() = default;

    UnixSocketClient(const UnixSocketClient&) = delete;
    UnixSocketClient(UnixSocketClient&&) = default;
    UnixSocketClient& operator=(const UnixSocketClient&) = delete;
    UnixSocketClient& operator=(UnixSocketClient&&) = default;
    ~UnixSocketClient();

    void connect(std::string_view filename);
    void send(std::span<const struct iovec> logs);
    void close();

private:
    engine::io::Socket socket_;
};

class UnixSocketSink final : public BaseSink {
public:
    explicit UnixSocketSink(std::string_view filename)
        : filename_{filename}
    {
        client_.connect(filename_);
    }

    void Write(std::span<const struct iovec> logs) final;

    void Close();

private:
    const std::string filename_;
    impl::UnixSocketClient client_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
