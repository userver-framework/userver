#include <userver/storages/sqlite/component.hpp>

#include <memory>

#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>
#include "userver/storages/sqlite/connection.hpp"

#include <userver/components/component.hpp>

#include <sqlite3.h>
#include <sqlite3ext.h>

USERVER_NAMESPACE_BEGIN

namespace components {

SQLite::SQLite(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      name_{config.Name()},
      connection_(std::make_shared<storages::sqlite::Connection>()) {}

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
    path_to_db:
        type: string
        description: path to .db file or `::memory` for in-memory mode
)");
}

}  // namespace components

USERVER_NAMESPACE_END
