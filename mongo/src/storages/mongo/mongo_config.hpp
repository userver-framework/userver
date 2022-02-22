#pragma once

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

class Config {
 public:
  uint32_t default_max_time_ms{0};

  Config() = default;
  Config(const dynamic_config::DocsMap& docs_map);
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
