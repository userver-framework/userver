#include <userver/storages/redis/impl/transaction_subrequest_data.hpp>

#include <fmt/format.h>

#include <userver/storages/redis/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

[[noreturn]] void ThrowTransactionNotStarted(std::string_view description) {
  throw NotStartedTransactionException(
      fmt::format("trying to {} transaction's subcommand result before calling "
                  "Exec() + Get() for the entire transaction",
                  description));
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
