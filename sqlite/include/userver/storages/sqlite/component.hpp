#pragma once

/// @file userver/storages/sqlite/component.hpp
/// @brief components::SQLite

#include <userver/components/component_base.hpp>

#include <userver/utils/statistics/entry.hpp>

#include <userver/storages/sqlite/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief SQLite client component
///
/// Provides access to a SQLite connections via storages::sqlite::Client.
///
/// ## Static configuration example:
///
/// ```
///  # yaml
///  sqlite-dv:
///    db-path: "/tmp/kv.db"
///    task_processor: fs-task-processor
///    journal_mode: wal
///    busy_timeout: 1000 # ms
///    read_only: false
///    initial_read_only_pool_size: 4
///    max_read_only_pool_size: 16
///    persistent_prepared_statements: true
///    cache_size: -65536 # KiB
///    foreign_keys: true
/// ```
/// You must specify either `db-path`.
///
/// You must specify `blocking_task_processor` as well.
///
/// Please see [SQLite documentation](https://www.sqlite.org/pragma.html)
/// on connection strings.
///
/// ## Static options:
/// Name                               | Description                                                                       | Default value
/// ---------------------------------- | --------------------------------------------------------------------------------- | ---------------
/// fs-task-processor                  | name of the task processor to handle the blocking file operations                 | engine::current_task::GetBlockingTaskProcessor()
/// db-path                            | path to the database file or `::memory::` for in-memory mode                      | -
/// create_file                        | сreate the database file if it does not exist at the specified path               | true
/// is_read_only                       | open the database in read-only mode                                               | false
/// shared_cache                       | enable shared in-memory cache for the database                                    | false
/// read_uncommitted                   | allow reading uncommitted data (requires shared_cache)                            | false
/// journal_mode                       | mode for database journaling                                                      | wal
/// busy_timeout                       | timeout duration (in milliseconds) to wait when the database is busy              | 5000
/// foreign_keys                       | enable enforcement of foreign key constraints                                     | true
/// synchronous                        | set the level of synchronization to ensure data durability                        | normal
/// cache_size                         | maximum cache size, specified in number of pages or in kibibytes (negative value) | -2000
/// journal_size_limit                 | limit the size of rollback-journal and WAL files (in bytes)                       | 67108864
/// mmap_size                          | maximum number of bytes allocated for memory-mapped I/O                           | 30000000000
/// page_size                          | size of a database page (in bytes)                                                | 4096
/// temp_store                         | storage location for temporary tables and indexes                                 | memory
/// persistent-prepared-statements     | cache prepared statements for reuse                                               | true
/// max_prepared_cache_size            | maximum number of prepared statements to cache                                    | 200
/// initial_read_only_pool_size        | initial size of the read-only connection pool                                     | 5
/// max_read_only_pool_size            | maximum size of the read-only connection pool                                     | 10

// clang-format on

class SQLite final : public components::ComponentBase {
public:
    /// Component constructor
    SQLite(const ComponentConfig&, const ComponentContext&);

    /// Component destructor
    ~SQLite() override;

    /// Client accessor
    storages::sqlite::ClientPtr GetClient() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    const storages::sqlite::settings::SQLiteSettings settings_;
    engine::TaskProcessor& fs_task_processor_;
    const storages::sqlite::ClientPtr client_;
    utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<SQLite> = true;

}  // namespace components

USERVER_NAMESPACE_END
