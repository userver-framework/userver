#pragma once

/// @file userver/storages/sqlite/component.hpp
/// @brief @copybrief components::SQLite

#include <userver/components/component_base.hpp>

#include <userver/storages/sqlite/client.hpp>

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
/// shared_cashe                       | open database with shared in-memory cashe                      | false
/// journal_mode                       | journal mode                                                   | wal
/// busy_timeout                       | queries busy timeout                                           | 5000
/// foreign_keys                       | enable foreign keys                                            | true
/// synchronous                        | durability level                                               | normal
/// cache_size                         | maximum cache size. In page or in kibibytes (negative)         | -2000
/// journal_size_limit                 | limit the size of rollback-journal and WAL files               | 67108864
/// mmap_size                          | max number of bytes that are set aside for memory-mapped I/O   | 134217728
/// page_size                          | page size of the database                                      | 4096
/// temp_store                         | where temporary tables and indices are stored                  | memory
/// persistent-prepared-statements     | cache prepared statements or not                               | true
/// max_prepared_cache_size            | prepared statements cache size limit                           | 200
/// initial_read_only_pool_size        | initial read only connection pool size                         | 5
/// max_read_only_pool_size            | maximum read only connection pool size                         | 10

// clang-format on

class SQLite final : public components::ComponentBase {
 public:
  /// Component constructor
  SQLite(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~SQLite() override;

  storages::sqlite::ClientPtr GetClient() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string name_;
  const storages::sqlite::ClientPtr client_;
};

template <>
inline constexpr bool kHasValidate<SQLite> = true;

}  // namespace components

USERVER_NAMESPACE_END
