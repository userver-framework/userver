#include <storages/mongo/cdriver/pool_access.hpp>

#include <type_traits>
#include <utility>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

BoundClient::BoundClient(CDriverPoolImpl::BoundClientPtr client)
    : client_(std::move(client))
{}
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
BoundClient::BoundClient(cdriver_experimental::CDriverPoolImpl::BoundClientPtr client)
    : client_(std::move(client))
{}
#endif

BoundClient BoundClient::Borrowed(const BoundClient& client) {
    return std::visit(
        [](const auto& ptr) { return BoundClient{std::decay_t<decltype(ptr)>::Borrowed(ptr)}; },
        client.client_
    );
}

BoundClient::operator bool() const {
    return std::visit([](const auto& ptr) { return static_cast<bool>(ptr); }, client_);
}

mongoc_client_t* BoundClient::get() {
    return std::visit([](auto& ptr) { return ptr.get(); }, client_);
}

void BoundClient::reset() {
    std::visit([](auto& ptr) { ptr.reset(); }, client_);
}

stats::EventStats BoundClient::GetEventStatsSnapshot() {
    return std::visit(
        [](auto& ptr) {
            if constexpr (std::is_same_v<std::decay_t<decltype(ptr)>, CDriverPoolImpl::BoundClientPtr>) {
                return ptr.GetEventStatsSnapshot();
            } else {
                return stats::GetTaskEventStats();
            }
        },
        client_
    );
}

bool BoundClient::ShouldAccountErrorsWithoutEvents() const {
    return !std::holds_alternative<CDriverPoolImpl::BoundClientPtr>(client_);
}

PoolAccess::PoolAccess(const PoolImplPtr& pool) {
    if (auto* legacy = dynamic_cast<CDriverPoolImpl*>(pool.get())) {
        pool_ = legacy;
        return;
    }
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    if (auto* experimental = dynamic_cast<cdriver_experimental::CDriverPoolImpl*>(pool.get())) {
        pool_ = experimental;
        return;
    }
#endif
    UINVARIANT(false, "Unsupported MongoDB pool implementation");
}

BoundClient PoolAccess::Acquire() const {
    return std::visit([](auto* pool) { return BoundClient{pool->Acquire()}; }, pool_);
}

const std::optional<std::chrono::seconds>& PoolAccess::GetMaxReplicationLag() const {
    return std::visit(
        [](const auto* pool) -> const std::optional<std::chrono::seconds>& { return pool->GetMaxReplicationLag(); },
        pool_
    );
}

bool PoolAccess::IsBulkWriteSupported(const BoundClient& client) const {
    return std::visit(
        [&client](const auto* pool) {
            using Pool = std::remove_cv_t<std::remove_pointer_t<decltype(pool)>>;
            const auto& ptr = std::get<typename Pool::BoundClientPtr>(client.client_);
            if constexpr (std::is_same_v<Pool, CDriverPoolImpl>) {
                UASSERT(ptr);
                return pool->IsBulkWriteSupported();
            } else {
                return pool->IsBulkWriteSupported(ptr);
            }
        },
        pool_
    );
}

void PoolAccess::RecheckBulkWriteSupport(BoundClient& client) const {
    std::visit(
        [&client](auto* pool) {
            using Pool = std::remove_pointer_t<decltype(pool)>;
            auto& ptr = std::get<typename Pool::BoundClientPtr>(client.client_);
            if constexpr (std::is_same_v<Pool, CDriverPoolImpl>) {
                pool->RecheckBulkWriteSupport(ptr.get());
            } else {
                pool->RecheckBulkWriteSupport(ptr);
            }
        },
        pool_
    );
}

void PoolAccess::MarkBulkWriteUnsupported(const BoundClient& client) const {
    std::visit(
        [&client](auto* pool) {
            using Pool = std::remove_pointer_t<decltype(pool)>;
            const auto& ptr = std::get<typename Pool::BoundClientPtr>(client.client_);
            if constexpr (std::is_same_v<Pool, CDriverPoolImpl>) {
                UASSERT(ptr);
                pool->MarkBulkWriteUnsupported();
            } else {
                pool->MarkBulkWriteUnsupported(ptr);
            }
        },
        pool_
    );
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
