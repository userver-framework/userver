#pragma once

#include <memory>
#include <string>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

struct ConnectionConfig {
  size_t in_buffer_size = 32 * 1024;
  size_t requests_queue_size_threshold = 100;
  std::chrono::seconds keepalive_timeout{10 * 60};
};

ConnectionConfig Parse(const yaml_config::YamlConfig& value,
                       formats::parse::To<ConnectionConfig>);

}  // namespace server::net

USERVER_NAMESPACE_END
