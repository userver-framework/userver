#include <userver/storages/redis/impl/transaction_subrequest_data.hpp>

#include <fmt/format.h>

#include <userver/storages/redis/transaction.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

[[noreturn]] void ThrowTransactionNotStarted(std::string_view description) {
    auto message = fmt::format(
        "Trying to {} transaction's subcommand result before calling Exec() + Get() for the entire transaction",
        description
    );

    UASSERT_MSG(false, message);
    throw NotStartedTransactionException(std::move(message));
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
