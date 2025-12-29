#pragma once

/// @file userver/storages/sqlite/component.hpp
/// @brief components::SQLite

#include <userver/components/component_base.hpp>

#include <userver/utils/statistics/entry.hpp>

#include <userver/storages/sqlite/client.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

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
/// You must specify `db-path`.
///
/// Please see [SQLite documentation](https://www.sqlite.org/pragma.html)
/// on connection strings.
///
/// ## Static options of components::SQLite :
/// @include{doc} scripts/docs/en/components_schema/sqlite/src/storages/sqlite/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
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
