#pragma once

#include <chrono>
#include <optional>
#include <variant>

#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/features.hpp>
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
#include <storages/mongo/cdriver_experimental/pool_impl.hpp>
#endif

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

class BoundClient final {
public:
    explicit BoundClient(CDriverPoolImpl::BoundClientPtr client);
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    explicit BoundClient(cdriver_experimental::CDriverPoolImpl::BoundClientPtr client);
#endif
    static BoundClient Borrowed(const BoundClient& client);

    BoundClient(BoundClient&&) noexcept = default;
    BoundClient(const BoundClient&) = delete;
    BoundClient& operator=(const BoundClient&) = delete;
    BoundClient& operator=(BoundClient&&) = delete;

    explicit operator bool() const;
    mongoc_client_t* get();
    void reset();
    stats::EventStats GetEventStatsSnapshot();
    bool ShouldAccountErrorsWithoutEvents() const;

private:
    friend class PoolAccess;
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    using Clients =
        std::variant<CDriverPoolImpl::BoundClientPtr, cdriver_experimental::CDriverPoolImpl::BoundClientPtr>;
#else
    using Clients = std::variant<CDriverPoolImpl::BoundClientPtr>;
#endif
    Clients client_;
};

class PoolAccess final {
public:
    explicit PoolAccess(const PoolImplPtr& pool);

    BoundClient Acquire() const;
    const std::optional<std::chrono::seconds>& GetMaxReplicationLag() const;
    bool IsBulkWriteSupported(const BoundClient& client) const;
    void RecheckBulkWriteSupport(BoundClient& client) const;
    void MarkBulkWriteUnsupported(const BoundClient& client) const;

private:
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    using Pools = std::variant<CDriverPoolImpl*, cdriver_experimental::CDriverPoolImpl*>;
#else
    using Pools = std::variant<CDriverPoolImpl*>;
#endif
    Pools pool_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
