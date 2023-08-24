#pragma once

#include <userver/congestion_control/controllers/linear.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

struct CcConfig {
  CcConfig(const dynamic_config::DocsMap& docs_map)
      : config(docs_map.Get("POSTGRES_CONGESTION_CONTROL_SETTINGS")) {}

  congestion_control::v2::Config config;
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
