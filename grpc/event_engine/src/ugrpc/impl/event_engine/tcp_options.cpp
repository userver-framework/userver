#include <ugrpc/impl/event_engine/tcp_options.hpp>

#if defined(ARCADIA_ROOT) && defined(__linux__)

#include <algorithm>
#include <stdexcept>

#include <grpc/impl/channel_arg_names.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

PosixTcpOptions TcpOptionsFromEndpointConfig(const grpc_event_engine::experimental::EndpointConfig& config) {
    PosixTcpOptions options;
    const int reuse = config.GetInt(GRPC_ARG_ALLOW_REUSEPORT).value_or(1);
    if (reuse != 0 && reuse != 1) {
        throw std::invalid_argument("grpc.so_reuseport must be 0 or 1; correct the server channel argument");
    }
    options.allow_reuse_port = reuse != 0;
    options
        .tcp_max_read_buffer_size = config.GetInt(kTcpMaxReadBufferSizeArg).value_or(options.tcp_max_read_buffer_size);
    if (config.GetString(kTcpMaxReadBufferSizeArg) ||
        (options.tcp_max_read_buffer_size != PosixTcpOptions::kMaxReadBufferSizeUnset &&
         options.tcp_max_read_buffer_size <= 0))
    {
        throw std::invalid_argument(
            "grpc.experimental.tcp_max_read_buffer_size must be a positive integer byte count or -1; "
            "correct or remove the server channel argument"
        );
    }
    options.keep_alive_time_ms = std::max(0, config.GetInt(GRPC_ARG_KEEPALIVE_TIME_MS).value_or(0));
    options.keep_alive_timeout_ms = std::max(0, config.GetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS).value_or(0));
    options.tcp_receive_buffer_size = std::max(0, config.GetInt(GRPC_ARG_TCP_RECEIVE_BUFFER_SIZE).value_or(0));
    options.tcp_read_chunk_size = std::clamp(
        config.GetInt(GRPC_ARG_TCP_READ_CHUNK_SIZE).value_or(PosixTcpOptions::kDefaultReadChunkSize),
        1,
        PosixTcpOptions::kMaxReadChunkSize
    );
    return options;
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END

#endif
