#pragma once

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <limits>
#include <optional>

#include <grpc/event_engine/endpoint_config.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

inline constexpr char kTcpMaxReadBufferSizeArg[] = "grpc.experimental.tcp_max_read_buffer_size";

struct PosixTcpOptions {
    static constexpr int kMaxReadBufferSizeUnset = -1;
    static constexpr int kDefaultServerUserTimeoutMs = 20'000;
    static constexpr int kDefaultReadChunkSize = 8 * 1024;
    static constexpr int kMaxReadChunkSize = 32 * 1024 * 1024;

    int tcp_max_read_buffer_size{kMaxReadBufferSizeUnset};
    bool allow_reuse_port{true};
    int keep_alive_time_ms{0};
    int keep_alive_timeout_ms{0};
    int tcp_receive_buffer_size{0};
    int tcp_read_chunk_size{kDefaultReadChunkSize};
};

PosixTcpOptions TcpOptionsFromEndpointConfig(const grpc_event_engine::experimental::EndpointConfig& config);

inline std::optional<int> ServerTcpUserTimeoutMs(const PosixTcpOptions& options) noexcept {
    if (options.keep_alive_time_ms == std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    return options.keep_alive_timeout_ms > 0
               ? options.keep_alive_timeout_ms
               : PosixTcpOptions::kDefaultServerUserTimeoutMs;
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
