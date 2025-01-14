#include <iostream>
#include <userver/storages/sqlite/component.hpp>

#include <memory>

#include <sqlite3.h>
#include <sqlite3ext.h>

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include "userver/storages/sqlite/connection.hpp"
#include "userver/storages/sqlite/options.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

std::shared_ptr<storages::sqlite::Connection> CreateConnection(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  storages::sqlite::SQLiteSettings settings;
  settings.db_name = config["db-path"].As<std::string>();
  settings.create_file = config["create_file"].As<bool>();
  settings.read_mode =
      config["is_read_only"].As<bool>()
          ? storages::sqlite::SQLiteSettings::ReadMode::kReadOnly
          : storages::sqlite::SQLiteSettings::ReadMode::kReadWrite;
  settings.conn_settings.prepared_statements =
      config["persistent-prepared-statements"].As<bool>(true)
          ? storages::sqlite::ConnectionSettings::kCachePreparedStatements
          : storages::sqlite::ConnectionSettings::kNoPreparedStatements;
  settings.conn_settings.max_prepared_cache_size =
      config["max_prepared_cache_size"].As<std::size_t>(
          storages::sqlite::kDefaultMaxPreparedCacheSize);
  return std::make_shared<storages::sqlite::Connection>(
      settings,
      context.GetTaskProcessor(config["task_processor"].As<std::string>()));
}

}  // namespace

SQLite::SQLite(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      name_{config.Name()},
      connection_(CreateConnection(config, context)) {}

SQLite::~SQLite() = default;

storages::sqlite::ConnectionPtr SQLite::GetConnection() const {
  return connection_;
}

yaml_config::Schema SQLite::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: SQLite client component
additionalProperties: false
properties:
    task_processor:
        type: string
        description: name of the task processor to run the blocking file operations
    db-path:
        type: string
        description: path to database file or `::memory` for in-memory mode
    create_file:
        type: boolean
        description: create a file if one is not found along the db-path
    is_read_only:
        type: boolean
        description: defines database access as read-only
    persistent-prepared-statements:
        type: boolean
        description: cache prepared statements or not
        defaultDescription: true
    max_prepared_cache_size:
        type: integer
        description: prepared statements cache size limit
        defaultDescription: 200
)");
}

}  // namespace components

USERVER_NAMESPACE_END
