#pragma once

#include <memory>
#include <string>

#include <userver/http/http_version.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <engine/ev/thread.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

inline constexpr std::chrono::milliseconds kDefaultAbortCheckDelay{20};
static_assert(
    kDefaultAbortCheckDelay > engine::ev::kMinDurationToDefer,
    "The default is inefficient as it would cause ev_async_send calls"
);

struct Http2SessionConfig final {
    std::uint32_t max_concurrent_streams = 100;
    std::uint32_t max_frame_size = 1 << 14;
    std::uint32_t initial_window_size = 1 << 16;
};

struct ConnectionConfig {
    size_t in_buffer_size = 32 * 1024;
    size_t requests_queue_size_threshold = 100;
    std::chrono::seconds keepalive_timeout{10 * 60};
    std::chrono::milliseconds abort_check_delay{kDefaultAbortCheckDelay};
    USERVER_NAMESPACE::http::HttpVersion http_version = USERVER_NAMESPACE::http::HttpVersion::k11;
    Http2SessionConfig http2_session_config;
};

ConnectionConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<ConnectionConfig>);

Http2SessionConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<Http2SessionConfig>);

}  // namespace server::net

USERVER_NAMESPACE_END
