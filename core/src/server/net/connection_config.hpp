#pragma once

#include <memory>
#include <string>

#include <userver/yaml_config/yaml_config.hpp>

#include <engine/ev/thread.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

inline constexpr std::chrono::milliseconds kDefaultAbortCheckDelay{20};
static_assert(
    kDefaultAbortCheckDelay > engine::ev::kMinDurationToDefer,
    "The default is inefficient as it would cause ev_async_send calls");

struct ConnectionConfig {
  size_t in_buffer_size = 32 * 1024;
  size_t requests_queue_size_threshold = 100;
  std::chrono::seconds keepalive_timeout{10 * 60};
  std::chrono::milliseconds abort_check_delay{kDefaultAbortCheckDelay};
};

ConnectionConfig Parse(const yaml_config::YamlConfig& value,
                       formats::parse::To<ConnectionConfig>);

}  // namespace server::net

USERVER_NAMESPACE_END
