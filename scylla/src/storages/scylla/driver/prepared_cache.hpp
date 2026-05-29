#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include <cassandra.h>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

class PreparedStatementCache final {
public:
    PreparedStatementCache() = default;
    PreparedStatementCache(const PreparedStatementCache&) = delete;
    PreparedStatementCache& operator=(const PreparedStatementCache&) = delete;

    CassStatementPtr GetOrPrepare(CassSession* session, std::string_view query);

    void Clear() noexcept {
        std::unique_lock lock(mutex_);
        cache_.clear();
    }

private:
    mutable engine::SharedMutex mutex_;
    std::unordered_map<std::string, CassPreparedPtr, utils::StrCaseHash, std::equal_to<>> cache_;
};

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
