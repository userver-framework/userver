#pragma once

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

struct CcConfig {
  CcConfig(const dynamic_config::DocsMap& docs_map)
      : config(docs_map.Get("MONGO_CONGESTION_CONTROL_SETTINGS")) {}

  congestion_control::v2::Config config;
};

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
