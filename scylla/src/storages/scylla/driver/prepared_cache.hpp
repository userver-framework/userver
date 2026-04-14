#pragma once

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <cassandra.h>

#include <storages/scylla/driver/cass_wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

class PreparedStatementCache final {
public:
    PreparedStatementCache() = default;
    PreparedStatementCache(const PreparedStatementCache&) = delete;
    PreparedStatementCache& operator=(const PreparedStatementCache&) = delete;

    CassStatementPtr GetOrPrepare(CassSession* session, const std::string& query);

    void Clear() noexcept {
        std::unique_lock lock(mutex_);
        cache_.clear();
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, CassPreparedPtr> cache_;
};

}

USERVER_NAMESPACE_END
