#pragma once

/// @file userver/storages/sqlite/component.hpp
/// @brief @copybrief components::SQLite

#include <userver/components/component_base.hpp>

#include <userver/storages/sqlite/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief SQLite client component.
/// ## Static options:
/// Name                               | Description                                                    | Default value
/// ---------------------------------- | -------------------------------------------------------------- | ---------------
/// task_processor                     | name of the task processor to run the blocking file operations | -
/// db-path                            | path to database file or `::memory` for in-memory mode         | -
/// create_file                        | create a file if one is not found along the db-path            | true
/// is_read_only                       | defines database access as read-only                           | false

// clang-format on

// for mocked tests
class ISQLite {
 public:
  virtual ~ISQLite() = default;
  virtual storages::sqlite::ConnectionPtr GetConnection() const = 0;
};

class SQLite final : public components::ComponentBase, public ISQLite {
 public:
  /// Component constructor
  SQLite(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~SQLite() override;

  storages::sqlite::ConnectionPtr GetConnection() const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string name_;
  const storages::sqlite::ConnectionPtr connection_;
};

template <>
inline constexpr bool kHasValidate<SQLite> = true;

}  // namespace components

USERVER_NAMESPACE_END
