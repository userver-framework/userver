#include <userver/storages/sqlite/connection.hpp>

#include "userver/storages/sqlite/result_set.hpp"
#include "userver/storages/sqlite/transaction.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Connection::Connection(const settings::SQLiteSettings& settings [[maybe_unused]],
          const components::ComponentConfig& config [[maybe_unused]]) {
}

Connection::~Connection() = default;

Transaction Connection::Begin(std::string name [[maybe_unused]], const TransactionOptions&) {
  return Transaction{};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
