#include <userver/storages/sqlite/connection.hpp>

#include <optional>
#include "userver/storages/sqlite/result_set.hpp"

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

ResultSet Connection::DoExecute(OptionalCommandControl command_control
                                [[maybe_unused]],
                                const Query& query [[maybe_unused]],
                                std::optional<std::size_t> batch_size
                                [[maybe_unused]]) const {
  return ResultSet{};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
