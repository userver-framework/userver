#include <memory>
#include <userver/storages/sqlite/component.hpp>

#include <userver/components/component.hpp>

#include <sqlite3.h>
#include <sqlite3ext.h>

USERVER_NAMESPACE_BEGIN

namespace components {

SQLite::SQLite(const ComponentConfig& config,
                   const ComponentContext& context)
    : ComponentBase(config, context),
      name_{config.Name()} {}

storages::sqlite::ConnectionPtr SQLite::GetConnection() const {
  return std::make_shared<storages::sqlite::Connection>();
}

}  // namespace components

USERVER_NAMESPACE_END
