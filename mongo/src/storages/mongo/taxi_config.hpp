#pragma once

#include <taxi_config/value.hpp>

namespace storages::mongo::impl {

struct TaxiConfig {
  explicit TaxiConfig(const taxi_config::DocsMap&);

  bool connect_precheck_enabled{false};
};

}  // namespace storages::mongo::impl
