#pragma once

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

#include <storages/postgres/postgres_fwd.hpp>

namespace components {

// clang-format off
/// @brief PosgreSQL client component
/// ## Configuration example
///
/// ## Available options
///
///
///
// clang-format on

class Postgres : public LoggableComponentBase {
 public:
  /// Component constructor
  Postgres(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~Postgres();

  /// Get cluster by alias
  storages::postgres::ClusterPtr GetCluster(const std::string& alias) const;
};

}  // namespace components
