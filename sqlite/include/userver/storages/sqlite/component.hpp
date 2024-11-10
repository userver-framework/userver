#pragma once

/// @file userver/storages/sqlite/component.hpp
/// @brief @copybrief components::SQLite

#include <userver/components/component_base.hpp>

#include <userver/storages/sqlite/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

class SQLite final : public components::ComponentBase {
 public:
  /// Component constructor
  SQLite(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~SQLite() override;

  storages::sqlite::ConnectionPtr GetConnection() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string name_;
  const storages::sqlite::ConnectionPtr connection_;
};

template <>
inline constexpr bool kHasValidate<SQLite> = true;

}  // namespace components

USERVER_NAMESPACE_END
