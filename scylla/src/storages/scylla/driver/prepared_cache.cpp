#include "prepared_cache.hpp"

#include <mutex>

#include <userver/tracing/span.hpp>

#include <storages/scylla/driver/async_future.hpp>
#include <storages/scylla/driver/scylla_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

CassStatementPtr PreparedStatementCache::GetOrPrepare(CassSession* session, std::string_view query) {
    {
        std::shared_lock lock(mutex_);
        const auto it = cache_.find(query);
        if (it != cache_.end()) {
            return CassStatementPtr{cass_prepared_bind(it->second.get())};
        }
    }

    tracing::Span span("scylla_prepare");
    CassFuturePtr prep_future(cass_session_prepare_n(session, query.data(), query.size()));
    AsyncWaitFuture(prep_future.get());
    CheckFuture(prep_future.get(), "prepare");

    CassPreparedPtr prepared(cass_future_get_prepared(prep_future.get()));

    std::unique_lock lock(mutex_);
    auto [it, inserted] = cache_.try_emplace(std::string{query}, std::move(prepared));
    return CassStatementPtr{cass_prepared_bind(it->second.get())};
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
