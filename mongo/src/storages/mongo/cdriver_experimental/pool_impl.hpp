#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <mongoc/mongoc.h>

#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/pool_impl.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver_experimental {

using cdriver::UriPtr;

class CDriverPoolImpl final : public PoolImpl {
public:
    enum class BulkWriteSupport { kUnknown, kSupported, kUnsupported };

    struct ConnectionPoolDeleter final {
        void operator()(mongoc_client_pool_t* pool) const noexcept {
            if (pool) {
                mongoc_client_pool_destroy(pool);
            }
        }
    };

    class ConnectionPool final {
    public:
        struct AsyncStreamInitiatorData {
            clients::dns::Resolver* dns_resolver;
            mongoc_ssl_opt_t ssl_opt;
        };

        ConnectionPool(
            std::string connection_string,
            UriPtr uri,
            clients::dns::Resolver* dns_resolver,
            std::shared_ptr<stats::ApmStats> apm_stats,
            std::shared_ptr<stats::PoolConnectStatistics> pool_stats
        );

        ~ConnectionPool();

        void RefreshStatistics();
        void SetMaxSize(std::uint32_t max_size);

        mongoc_client_pool_t* GetNativePtr() const { return pool_.get(); }
        const std::string& GetConnectionString() const { return connection_string_; }
        std::string GetDatabaseName() const { return mongoc_uri_get_database(uri_.get()); }
        std::int32_t GetMaxConnecting() const {
            return mongoc_uri_get_option_as_int32(uri_.get(), MONGOC_URI_MAXCONNECTING, 2);
        }
        AsyncStreamInitiatorData& GetInitiatorData() { return init_data_; }
        stats::ConnStats* GetStatsPtr() { return &stats_; }

        BulkWriteSupport GetBulkWriteSupport() const noexcept;
        bool MarkBulkWriteUnsupported() noexcept;
        bool SetBulkWriteSupportIfUnknown(BulkWriteSupport support) noexcept;

    private:
        std::atomic<BulkWriteSupport> bulk_write_support_{BulkWriteSupport::kUnknown};
        engine::Mutex statistics_mutex_;
        mongoc_client_pool_stats_t last_statistics_{};
        const std::string connection_string_;
        UriPtr uri_;
        AsyncStreamInitiatorData init_data_;
        std::shared_ptr<stats::ApmStats> apm_stats_;
        std::shared_ptr<stats::PoolConnectStatistics> pool_stats_;
        stats::ConnStats stats_;
        std::unique_ptr<mongoc_client_pool_t, ConnectionPoolDeleter> pool_;
    };

    using ConnPoolPtr = std::shared_ptr<ConnectionPool>;

    class Connection final {
    public:
        Connection(ConnPoolPtr pool, mongoc_client_t* client)
            : pool_(std::move(pool)),
              client_(client)
        {
            UASSERT(client_);
        }
        ~Connection() { Push(); }

        Connection(Connection&& other) = delete;
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection& operator=(const Connection&&) = delete;

        mongoc_client_t* GetNativePtr() const { return client_; }
        const ConnPoolPtr& GetPool() const { return pool_; }
        void Push();

    private:
        ConnPoolPtr pool_;
        mongoc_client_t* client_;
    };

    using ConnPtr = std::unique_ptr<Connection>;

    class BoundClientPtr final {
    public:
        BoundClientPtr(ConnPtr ptr, std::shared_ptr<engine::CancellableSemaphore> semaphore) noexcept
            : conn_(ptr.release()), semaphore_(std::move(semaphore)) {
            UASSERT(conn_);
            UASSERT(semaphore_);
        }

        /// Non-owning view of an owning BoundClientPtr.
        /// Caller must ensure `client` outlives the returned view.
        /// `client` must be owning; borrowing from a borrowed view is not supported.
        static BoundClientPtr Borrowed(const BoundClientPtr& client) noexcept {
            UASSERT(client.conn_);
            UASSERT_MSG(client.semaphore_, "Borrowed() may only be called on an owning BoundClientPtr");
            return BoundClientPtr{client.conn_};
        }

        BoundClientPtr(BoundClientPtr&& o) noexcept
            : conn_(std::exchange(o.conn_, nullptr)),
              semaphore_(std::move(o.semaphore_)) {}

        BoundClientPtr(const BoundClientPtr&) = delete;
        BoundClientPtr& operator=(const BoundClientPtr&) = delete;
        BoundClientPtr& operator=(BoundClientPtr&&) = delete;

        ~BoundClientPtr() { ReturnIfOwning(); }

        explicit operator bool() const { return conn_ != nullptr; }
        mongoc_client_t* get() const { return conn_->GetNativePtr(); }
        const ConnPoolPtr& GetPool() const { return conn_->GetPool(); }

        void reset() {
            ReturnIfOwning();
            conn_ = nullptr;
            semaphore_.reset();
        }

    private:
        explicit BoundClientPtr(Connection* borrowed) noexcept : conn_(borrowed) {
            UASSERT(conn_);
        }

        void ReturnIfOwning() noexcept {
            if (conn_ && semaphore_) {
                ConnPtr owned{std::exchange(conn_, nullptr)};
                owned.reset();
                semaphore_->unlock_shared();
                semaphore_.reset();
            }
        }

        Connection* conn_ = nullptr;
        std::shared_ptr<engine::CancellableSemaphore> semaphore_;
    };

    CDriverPoolImpl(
        utils::ResourceScopeStorage& scopes,
        std::string id,
        const std::string& uri_string,
        const PoolConfig& config,
        clients::dns::Resolver* dns_resolver,
        dynamic_config::Source config_source
    );

    ~CDriverPoolImpl() override;

    std::string DefaultDatabaseName() const override;
    const std::optional<std::chrono::seconds>& GetMaxReplicationLag() const;

    void Ping() override;

    size_t InUseApprox() const override;
    size_t SizeApprox() const override;
    size_t MaxSize() const override;
    const stats::ApmStats& GetApmStats() const override;
    void SetMaxSize(size_t max_size) override;

    /// @throws CancelledException, PoolOverloadException
    BoundClientPtr Acquire();

    bool IsBulkWriteSupported(const BoundClientPtr& client) const;

    void RecheckBulkWriteSupport(const BoundClientPtr& client);

    void MarkBulkWriteUnsupported(const BoundClientPtr& client);

    void SetPoolSettings(const PoolSettings& pool_settings) override;

    void SetConnectionString(const std::string& connection_string) override;

private:
    class GenerationCleanup;

    ConnPtr Pop();
    ConnPtr TryPop(const ConnPoolPtr& pool);
    ConnPtr WrapClient(const ConnPoolPtr& pool, mongoc_client_t* client);
    ConnPoolPtr CreatePool(const std::string& connection_string, std::size_t connecting_limit);
    void ReplacePool(ConnPoolPtr replacement);
    ConnPoolPtr GetPool() const;

    const std::string app_name_;
    clients::dns::Resolver* const dns_resolver_;

    std::atomic<size_t> max_size_;
    std::atomic<size_t> idle_limit_;
    std::atomic<size_t> connecting_limit_;
    std::shared_ptr<engine::CancellableSemaphore> in_use_semaphore_;
    mutable engine::Mutex settings_mutex_;
    engine::Mutex reload_mutex_;
    const PoolConfig pool_config_;
    std::shared_ptr<stats::ApmStats> apm_stats_ = std::make_shared<stats::ApmStats>();
    std::shared_ptr<GenerationCleanup> generation_cleanup_;

    ConnPoolPtr pool_;
};

}  // namespace storages::mongo::impl::cdriver_experimental

USERVER_NAMESPACE_END
