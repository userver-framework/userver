#pragma once

#include <userver/taxi_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

class Config {
 public:
  uint32_t default_max_time_ms{0};

  Config() = default;
  Config(const taxi_config::DocsMap& docs_map);
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
