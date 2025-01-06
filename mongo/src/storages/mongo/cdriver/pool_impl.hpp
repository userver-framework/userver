#pragma once

#include <chrono>

#include <mongoc/mongoc.h>
#include <moodycamel/concurrentqueue.h>

#include <storages/mongo/cdriver/async_stream.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/dynamic_config.hpp>
#include <storages/mongo/pool_impl.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

class CDriverPoolImpl final : public PoolImpl {
public:
    struct ClientDeleter final {
        void operator()(mongoc_client_t* client) const noexcept {
            if (client) {
                mongoc_client_destroy(client);
            }
        }
    };

    class Connection final {
    public:
        Connection(mongoc_client_t* client, stats::ApmStats* apm_stats) : client_(client) {
            UASSERT(client_);
            UASSERT(apm_stats);
            stats_.apm_stats_ = apm_stats;
        }

        Connection(Connection&& other) = delete;
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection& operator=(const Connection&&) = delete;

        mongoc_client_t* GetNativePtr() { return client_.get(); }

        stats::ConnStats* GetStatsPtr() { return &stats_; }

    private:
        stats::ConnStats stats_;
        std::unique_ptr<mongoc_client_t, ClientDeleter> client_{nullptr};
    };

    using ConnPtr = std::unique_ptr<Connection>;

    class BoundClientPtr final {
    public:
        explicit BoundClientPtr(ConnPtr ptr, CDriverPoolImpl* pool) noexcept : ptr_(std::move(ptr)), pool_(pool) {
            UASSERT(pool_);
            UASSERT(ptr_);
        }

        BoundClientPtr(BoundClientPtr&& o) noexcept = default;

        BoundClientPtr(const BoundClientPtr&) = delete;
        BoundClientPtr& operator=(const BoundClientPtr&) = delete;
        BoundClientPtr& operator=(const BoundClientPtr&&) = delete;

        explicit operator bool() { return !!ptr_; }

        stats::EventStats GetEventStatsSnapshot() {
            UASSERT(ptr_);
            return ptr_->GetStatsPtr()->event_stats_;
        }

        void reset() {
            UASSERT(ptr_);
            pool_->Push(std::move(ptr_));
        }

        ~BoundClientPtr() {
            if (ptr_) pool_->Push(std::move(ptr_));
        }

        mongoc_client_t* get() { return ptr_->GetNativePtr(); }

    private:
        ConnPtr ptr_;
        CDriverPoolImpl* pool_;
    };

    CDriverPoolImpl(
        std::string id,
        const std::string& uri_string,
        const PoolConfig& config,
        clients::dns::Resolver* dns_resolver,
        dynamic_config::Source config_source
    );

    ~CDriverPoolImpl() override;

    const std::string& DefaultDatabaseName() const override;

    void Ping() override;

    size_t InUseApprox() const override;
    size_t SizeApprox() const override;
    size_t MaxSize() const override;
    const stats::ApmStats& GetApmStats() const override;
    void SetMaxSize(size_t max_size) override;

    /// @throws CancelledException, PoolOverloadException
    BoundClientPtr Acquire();

    void SetPoolSettings(const PoolSettings& pool_settings) override;

private:
    ConnPtr Pop();
    void Push(ConnPtr) noexcept;
    void Drop(ConnPtr) noexcept;

    ConnPtr TryGetIdle();
    ConnPtr Create();

    void DoMaintenance();

    const std::string app_name_;
    std::string default_database_;
    UriPtr uri_;
    AsyncStreamInitiatorData init_data_;

    std::atomic<size_t> max_size_;
    std::atomic<size_t> idle_limit_;
    const std::chrono::milliseconds queue_timeout_;
    std::atomic<size_t> size_;
    engine::Semaphore in_use_semaphore_;
    engine::Semaphore connecting_semaphore_;
    moodycamel::ConcurrentQueue<ConnPtr> queue_;
    // ApmStats must be after the queue_
    stats::ApmStats apm_stats_;
    utils::PeriodicTask maintenance_task_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
