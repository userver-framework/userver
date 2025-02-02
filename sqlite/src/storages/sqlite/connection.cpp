#include <userver/storages/sqlite/connection.hpp>

#include <memory>
#include <optional>

#include <sqlite3.h>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include "userver/storages/sqlite/impl/connection_impl.hpp"
#include "userver/storages/sqlite/transaction.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Connection::Connection(const SQLiteSettings& settings,
                       engine::TaskProcessor& blocking_task_processor)
    : pimpl_(std::make_shared<impl::ConnectionImpl>(settings,
                                                    blocking_task_processor)) {}
Connection::~Connection() = default;

Transaction Connection::Begin(std::string name,
                              const TransactionOptions& options) const {
  return Begin(std::nullopt, name, options);
}

Transaction Connection::Begin(OptionalCommandControl command_control
                              [[maybe_unused]],
                              std::string name [[maybe_unused]],
                              const TransactionOptions& options) const {
  // TODO: use name
  return Transaction{pimpl_, options};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
