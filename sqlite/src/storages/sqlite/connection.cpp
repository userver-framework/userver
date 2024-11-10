#include <userver/storages/sqlite/connection.hpp>

#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Connection::Connection(const settings::SQLiteSettings& settings
                       [[maybe_unused]],
                       const components::ComponentConfig& config
                       [[maybe_unused]]) {}

Connection::~Connection() = default;

Transaction Connection::Begin(std::string name,
                              const TransactionOptions& options) const {
  return Begin(std::nullopt, name, options);
}

Transaction Connection::Begin(OptionalCommandControl command_control
                              [[maybe_unused]],
                              std::string name [[maybe_unused]],
                              const TransactionOptions& options
                              [[maybe_unused]]) const {
  return Transaction{};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
