#include <storages/mongo/cdriver_experimental/pool_impl.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string_view>

#include <bson/bson.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <mongoc/mongoc.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/formats/bson.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/traceful_exception.hpp>

#include <storages/mongo/cdriver/async_stream.hpp>
#include <storages/mongo/stats.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>

#include <dynamic_config/variables/USERVER_DEADLINE_PROPAGATION_ENABLED.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver_experimental {

namespace {

constexpr std::uint32_t kRetiredPoolMaxSize = 1;

mongoc_stream_t* MakeAsyncStream(
    const mongoc_uri_t* uri,
    const mongoc_host_list_t* host,
    void* user_data,
    bson_error_t* error
) noexcept {
    auto& data = *static_cast<CDriverPoolImpl::ConnectionPool::AsyncStreamInitiatorData*>(user_data);
    return cdriver::MakeAsyncStreamForNativePool(uri, host, data.dns_resolver, data.ssl_opt, error);
}

using RealMilliseconds = std::chrono::duration<double, std::milli>;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void SetNativeIdleLimit(mongoc_client_pool_t* pool, std::uint32_t idle_limit) {
    mongoc_client_pool_min_size(pool, idle_limit);
}
#pragma GCC diagnostic pop

int32_t CheckedDurationMs(const std::chrono::milliseconds& timeout, const char* name) {
    auto timeout_ms = timeout.count();
    if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int32_t>::max()) {
        throw InvalidConfigException("Bad value ") << timeout_ms << "ms for '" << name << '\'';
    }
    return timeout_ms;
}

int32_t CheckedDurationSeconds(const std::chrono::seconds& timeout, const char* name) {
    auto timeout_sec = timeout.count();
    if (timeout_sec < 0 || timeout_sec > std::numeric_limits<int32_t>::max()) {
        throw InvalidConfigException("Bad value ") << timeout_sec << "s for '" << name << '\'';
    }
    return timeout_sec;
}

#ifdef MONGOC_BULKWRITE_H

constexpr std::int32_t kBulkWriteMinWireVersion = 25;

std::optional<std::int32_t> GetMaxWireVersion(mongoc_client_t* client) {
    MongoError error;
    const cdriver::ServerDescriptionPtr description{
        mongoc_client_select_server(client, /*for_writes=*/true, /*prefs=*/nullptr, error.GetNative())
    };
    if (!description) {
        LOG_LIMITED_WARNING() << "Cannot detect MongoDB server version: " << error.Message();
        return std::nullopt;
    }

    const bson_t* hello_response = mongoc_server_description_hello_response(description.get());
    bson_iter_t iter;
    if (!hello_response || !bson_iter_init_find(&iter, hello_response, "maxWireVersion") ||
        !BSON_ITER_HOLDS_INT32(&iter))
    {
        LOG_LIMITED_WARNING() << "No 'maxWireVersion' in the MongoDB server handshake response";
        return std::nullopt;
    }
    return bson_iter_int32(&iter);
}

#endif  // MONGOC_BULKWRITE_H

bool HasOption(const cdriver::UriPtr& uri, const char* opt) {
    const bson_t* options = mongoc_uri_get_options(uri.get());
    bson_iter_t it;
    return bson_iter_init_find_case(&it, options, opt);
}

cdriver::UriPtr MakeUri(
    const std::string& pool_id,
    const std::string& uri_string,
    const PoolConfig& config,
    std::size_t connecting_limit
) {
    MongoError parse_error;
    cdriver::UriPtr uri(mongoc_uri_new_with_error(uri_string.c_str(), parse_error.GetNative()));
    if (!uri) {
        throw InvalidConfigException("Bad MongoDB uri for pool '") << pool_id << "': " << parse_error.Message();
    }
    if (!HasOption(uri, MONGOC_URI_MAXCONNECTING)) {
        if (connecting_limit == 0 || connecting_limit > std::numeric_limits<std::int32_t>::max()) {
            throw InvalidConfigException("MongoDB connecting_limit must be between 1 and INT32_MAX for pool '"
            ) << pool_id
              << '\'';
        }
        UINVARIANT(
            mongoc_uri_set_option_as_int32(uri.get(), MONGOC_URI_MAXCONNECTING, connecting_limit),
            "MongoDB driver must support maxConnecting"
        );
    }
    mongoc_uri_set_option_as_int32(uri.get(), MONGOC_URI_MINPOOLSIZE, 0);
    mongoc_uri_set_option_as_int32(
        uri.get(),
        MONGOC_URI_CONNECTTIMEOUTMS,
        CheckedDurationMs(config.conn_timeout, MONGOC_URI_CONNECTTIMEOUTMS)
    );
    if (utils::impl::kServerSelectionTimeoutExperiment.IsEnabled()) {
        mongoc_uri_set_option_as_int32(uri.get(), MONGOC_URI_SERVERSELECTIONTIMEOUTMS, 3000);
    }
    mongoc_uri_set_option_as_int32(
        uri.get(),
        MONGOC_URI_SOCKETTIMEOUTMS,
        CheckedDurationMs(config.so_timeout, MONGOC_URI_SOCKETTIMEOUTMS)
    );
    if (config.local_threshold) {
        mongoc_uri_set_option_as_int32(
            uri.get(),
            MONGOC_URI_LOCALTHRESHOLDMS,
            CheckedDurationMs(*config.local_threshold, MONGOC_URI_LOCALTHRESHOLDMS)
        );
    }
    if (config.max_replication_lag) {
        const auto max_repl_lag_sec = std::chrono::duration_cast<std::chrono::seconds>(*config.max_replication_lag);
        if (max_repl_lag_sec.count() < MONGOC_SMALLEST_MAX_STALENESS_SECONDS) {
            throw InvalidConfigException("Invalid max replication lag "
            ) << max_repl_lag_sec.count()
              << "s, must be at least " << MONGOC_SMALLEST_MAX_STALENESS_SECONDS << 's';
        }
        mongoc_uri_set_option_as_int32(
            uri.get(),
            MONGOC_URI_MAXSTALENESSSECONDS,
            CheckedDurationSeconds(max_repl_lag_sec, MONGOC_URI_MAXSTALENESSSECONDS)
        );
    }

    // Don't retry operation with single threaded mongo topology
    // It does usleep() in mongoc_topology_select_server_id()
    if (!HasOption(uri, MONGOC_URI_RETRYREADS)) {
        mongoc_uri_set_option_as_bool(uri.get(), MONGOC_URI_RETRYREADS, false);
    }

    // TODO: mongoc 1.15 has changed default of retryWrites to true but it
    // doesn't play well with mongos over standalone instance (testsuite)
    // TAXICOMMON-1455
    if (!HasOption(uri, MONGOC_URI_RETRYWRITES)) {
        mongoc_uri_set_option_as_bool(uri.get(), MONGOC_URI_RETRYWRITES, false);
    }

    return uri;
}

mongoc_ssl_opt_t MakeSslOpt(const mongoc_uri_t* uri) {
    mongoc_ssl_opt_t ssl_opt = *mongoc_ssl_opt_get_default();
    ssl_opt.pem_file = mongoc_uri_get_option_as_utf8(uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, nullptr);
    ssl_opt.pem_pwd = mongoc_uri_get_option_as_utf8(uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, nullptr);
    ssl_opt.ca_file = mongoc_uri_get_option_as_utf8(uri, MONGOC_URI_TLSCAFILE, nullptr);
    ssl_opt.weak_cert_validation = mongoc_uri_get_option_as_bool(uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false);
    ssl_opt.allow_invalid_hostname = mongoc_uri_get_option_as_bool(uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false);
    return ssl_opt;
}

std::string MakeQueueDeadlineMessage(std::optional<engine::Deadline::Duration> inherited_timeout) {
    if (!inherited_timeout) {
        return {};
    }
    return fmt::format(
        "Queue timeout set by deadline propagation: {}. ",
        std::chrono::duration_cast<std::chrono::milliseconds>(*inherited_timeout)
    );
}

void CheckTaskCancellation() {
    if (engine::current_task::ShouldCancel()) {
        throw CancelledException("Operation cancelled: ") << ToString(engine::current_task::CancellationReason());
    }
}

std::shared_lock<engine::CancellableSemaphore> WaitForPermit(
    engine::CancellableSemaphore& semaphore,
    engine::Deadline deadline
) {
    if (!semaphore.try_lock_shared_until(deadline)) {
        CheckTaskCancellation();
        return {};
    }
    std::shared_lock lock(semaphore, std::adopt_lock);
    CheckTaskCancellation();
    return lock;
}

void HandleCancellations(
    engine::Deadline& queue_deadline,
    std::optional<engine::Deadline::Duration>& inherited_timeout
) {
    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();

    if (inherited_deadline < queue_deadline) {
        queue_deadline = inherited_deadline;
        inherited_timeout = inherited_deadline.TimeLeftApprox();
        if (*inherited_timeout <= std::chrono::seconds{0}) {
            throw CancelledException(CancelledException::ByDeadlinePropagation{});
        }
    }

    CheckTaskCancellation();
}

stats::ConnStats& GetStats(void* stats_ptr) {
    UASSERT(stats_ptr);
    return *reinterpret_cast<stats::ConnStats*>(stats_ptr);
}

void CommandSucceeded(const mongoc_apm_command_succeeded_t* event) {
    stats::AccountTaskCommandSuccess();
    if (std::string_view{mongoc_apm_command_succeeded_get_command_name(event)} == "endSessions") {
        TESTPOINT("mongo-pool-end-sessions-succeeded", {});
    }
}

void CommandFailed(const mongoc_apm_command_failed_t*) { stats::AccountTaskCommandFailure(); }

void HeartbeatStarted(const mongoc_apm_server_heartbeat_started_t* event) {
    auto& stats = GetStats(mongoc_apm_server_heartbeat_started_get_context(event));
    ++stats.apm_stats->heartbeats.start;
    LOG_LIMITED_DEBUG() << mongoc_apm_server_heartbeat_started_get_host(event)->host_and_port << " heartbeat started";
}

void HeartbeatFinished(std::int64_t duration_us) {
    auto* span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
        const auto diff = std::chrono::duration_cast<RealMilliseconds>(std::chrono::microseconds{duration_us});
        span->AddTag("heartbeat_time", diff.count());
    }
}

void HeartbeatSuccess(const mongoc_apm_server_heartbeat_succeeded_t* event) {
    auto& stats = GetStats(mongoc_apm_server_heartbeat_succeeded_get_context(event));
    ++stats.apm_stats->heartbeats.success;
    LOG_LIMITED_DEBUG()
        << mongoc_apm_server_heartbeat_succeeded_get_host(event)->host_and_port << " heartbeat succeeded";
    HeartbeatFinished(mongoc_apm_server_heartbeat_succeeded_get_duration(event));
}

void HeartbeatFailed(const mongoc_apm_server_heartbeat_failed_t* event) {
    auto& stats = GetStats(mongoc_apm_server_heartbeat_failed_get_context(event));
    ++stats.apm_stats->heartbeats.failed;

    MongoError error;
    mongoc_apm_server_heartbeat_failed_get_error(event, error.GetNative());
    LOG_LIMITED_WARNING()
        << mongoc_apm_server_heartbeat_failed_get_host(event)->host_and_port
        << " heartbeat failed with error: " << error.Message();
    HeartbeatFinished(mongoc_apm_server_heartbeat_failed_get_duration(event));
}

std::string CreateTopologyChangeMessage(const mongoc_apm_topology_changed_t* event) {
    const mongoc_topology_description_t* prev_td = mongoc_apm_topology_changed_get_previous_description(event);
    const mongoc_topology_description_t* new_td = mongoc_apm_topology_changed_get_new_description(event);
    std::size_t nprev_server_desc{0};
    mongoc_server_description_t** prev_sds = mongoc_topology_description_get_servers(prev_td, &nprev_server_desc);
    std::size_t nnew_server_desc{0};
    mongoc_server_description_t** new_sds = mongoc_topology_description_get_servers(new_td, &nnew_server_desc);

    const utils::FastScopeGuard server_descriptions_guard{[&]() noexcept {
        mongoc_server_descriptions_destroy_all(prev_sds, nprev_server_desc);
        mongoc_server_descriptions_destroy_all(new_sds, nnew_server_desc);
    }};

    std::string topology_msg{fmt::format(
        "Topology changed: {} -> {}",
        mongoc_topology_description_type(prev_td),
        mongoc_topology_description_type(new_td)
    )};

    if (nprev_server_desc > 0) {
        topology_msg.append("\nPrevious servers:\n");
        for (std::size_t i = 0; i < nprev_server_desc; ++i) {
            auto server_name = fmt::format(
                "{} {},",
                mongoc_server_description_type(prev_sds[i]),
                mongoc_server_description_host(prev_sds[i])->host_and_port
            );
            topology_msg.append(fmt::to_string(server_name));
        }
    }

    if (nnew_server_desc > 0) {
        topology_msg.append("\nNew servers:\n");
        for (std::size_t i = 0; i < nnew_server_desc; ++i) {
            auto server_name = fmt::format(
                "{} {},",
                mongoc_server_description_type(new_sds[i]),
                mongoc_server_description_host(new_sds[i])->host_and_port
            );
            topology_msg.append(fmt::to_string(server_name));
        }
    }

    mongoc_read_prefs_t* prefs = mongoc_read_prefs_new(MONGOC_READ_SECONDARY);
    const utils::FastScopeGuard prefs_guard{[&prefs]() noexcept { mongoc_read_prefs_destroy(prefs); }};

#if MONGOC_CHECK_VERSION(1, 17, 0)
    if (mongoc_topology_description_has_readable_server(new_td, prefs)) {
#else
    if (mongoc_topology_description_has_readable_server(const_cast<mongoc_topology_description_t*>(new_td), prefs)) {
#endif
        topology_msg.append("\nSecondary AVAILABLE\n");
    } else {
        topology_msg.append("\nSecondary UNAVAILABLE\n");
    }

#if MONGOC_CHECK_VERSION(1, 17, 0)
    if (mongoc_topology_description_has_writable_server(new_td)) {
#else
    if (mongoc_topology_description_has_writable_server(const_cast<mongoc_topology_description_t*>(new_td))) {
#endif
        topology_msg.append("Primary AVAILABLE");
    } else {
        topology_msg.append("Primary UNAVAILABLE");
    }

    return topology_msg;
}

void TopologyChanged(const mongoc_apm_topology_changed_t* event) {
    auto& stats = GetStats(mongoc_apm_topology_changed_get_context(event));
    ++stats.apm_stats->topology.changed;

    LOG_INFO() << CreateTopologyChangeMessage(event);
}

void TopologyOpening(const mongoc_apm_topology_opening_t*) {
    LOG_DEBUG() << "The driver initializes a mongoc_topology_description_t";
}

void TopologyClosed(const mongoc_apm_topology_closed_t*) {
    LOG_DEBUG() << "The driver stops monitoring a server topology and destroys it";
}

}  // namespace

class CDriverPoolImpl::GenerationCleanup final {
public:
    ~GenerationCleanup() { Stop(); }

    void Dispose(std::unique_ptr<ConnectionPool> generation) noexcept {
        try {
            const std::lock_guard lock(mutex_);
            if (stopping_) {
                return;
            }
            tasks_.CriticalAsyncDetach("mongo_pool_generation_destroy", [generation = std::move(generation)]() mutable {
                const engine::TaskCancellationBlocker blocker;
                generation.reset();
            });
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Cannot schedule MongoDB pool generation cleanup: " << ex.what();
        }
    }

    void Stop() noexcept {
        {
            const std::lock_guard lock(mutex_);
            stopping_ = true;
        }
        tasks_.WaitAndDisposeSlow();
    }

private:
    engine::Mutex mutex_;
    bool stopping_{false};
    concurrent::BackgroundTaskStorage tasks_;
};

CDriverPoolImpl::ConnectionPool::ConnectionPool(
    std::string connection_string,
    cdriver::UriPtr uri,
    clients::dns::Resolver* dns_resolver,
    std::shared_ptr<stats::ApmStats> apm_stats,
    std::shared_ptr<stats::PoolConnectStatistics> pool_stats
)
    : connection_string_(std::move(connection_string)),
      uri_(std::move(uri)),
      init_data_{dns_resolver, MakeSslOpt(uri_.get())},
      apm_stats_(std::move(apm_stats)),
      pool_stats_(std::move(pool_stats))
{
    stats_.apm_stats = apm_stats_.get();
    bson_error_t error{};
    pool_.reset(mongoc_client_pool_new_with_error(uri_.get(), &error));
    if (!pool_) {
        throw InvalidConfigException("Failed to create mongo pool: ") << error.message;
    }
}

CDriverPoolImpl::ConnectionPool::~ConnectionPool() {
    TESTPOINT("mongo-pool-generation-destroying", {});
    RefreshStatistics();
    const auto remaining_clients = last_statistics_.clients_in_pool;
    if (remaining_clients == 0) {
        mongoc_client_pool_max_size(pool_.get(), 0);
    }
    pool_.reset();
    pool_stats_->closed.Add(stats::Rate{remaining_clients});
    pool_stats_->current_size.fetch_sub(remaining_clients, std::memory_order_relaxed);
}

CDriverPoolImpl::BulkWriteSupport CDriverPoolImpl::ConnectionPool::GetBulkWriteSupport() const noexcept {
    return bulk_write_support_.load(std::memory_order_relaxed);
}

bool CDriverPoolImpl::ConnectionPool::MarkBulkWriteUnsupported() noexcept {
    return bulk_write_support_.exchange(BulkWriteSupport::kUnsupported, std::memory_order_relaxed) ==
           BulkWriteSupport::kSupported;
}

bool CDriverPoolImpl::ConnectionPool::SetBulkWriteSupportIfUnknown(BulkWriteSupport support) noexcept {
    auto expected = BulkWriteSupport::kUnknown;
    return bulk_write_support_.compare_exchange_strong(expected, support, std::memory_order_relaxed);
}

void CDriverPoolImpl::ConnectionPool::RefreshStatistics() {
    const std::lock_guard lock(statistics_mutex_);
    mongoc_client_pool_stats_t statistics{};
    mongoc_client_pool_get_stats(pool_.get(), &statistics);
    const auto created = statistics.clients_created - last_statistics_.clients_created;
    const auto destroyed = statistics.clients_destroyed - last_statistics_.clients_destroyed;
    pool_stats_->created.Add(stats::Rate{created});
    pool_stats_->closed.Add(stats::Rate{destroyed});
    if (statistics.clients_in_pool >= last_statistics_.clients_in_pool) {
        pool_stats_->current_size
            .fetch_add(statistics.clients_in_pool - last_statistics_.clients_in_pool, std::memory_order_relaxed);
    } else {
        pool_stats_->current_size
            .fetch_sub(last_statistics_.clients_in_pool - statistics.clients_in_pool, std::memory_order_relaxed);
    }
    last_statistics_ = statistics;
}

void CDriverPoolImpl::ConnectionPool::SetMaxSize(std::uint32_t max_size) {
    mongoc_client_pool_max_size(pool_.get(), max_size);
    RefreshStatistics();
}

CDriverPoolImpl::CDriverPoolImpl(
    utils::ResourceScopeStorage& scopes,
    std::string id,
    const std::string& uri_string,
    const PoolConfig& config,
    clients::dns::Resolver* dns_resolver,
    dynamic_config::Source config_source
)
    : PoolImpl(scopes, std::move(id), config, config_source),
      app_name_(config.app_name),
      dns_resolver_(dns_resolver),
      max_size_(config.pool_settings.max_size),
      idle_limit_(config.pool_settings.idle_limit),
      connecting_limit_(config.pool_settings.connecting_limit),
      in_use_semaphore_(std::make_shared<engine::CancellableSemaphore>(config.pool_settings.max_size)),
      pool_config_(config),
      generation_cleanup_(std::make_shared<GenerationCleanup>())
{
    cdriver::GlobalInitializer::CheckInitialized();
    cdriver::GlobalInitializer::LogInitWarningsOnce();

    pool_ = CreatePool(uri_string, connecting_limit_);
    SetNativeIdleLimit(pool_->GetNativePtr(), idle_limit_.load());
    std::size_t i = 0;
    std::vector<ConnPtr> connections;
    try {
        const tracing::Span span("mongo_prepopulate");
        LOG_INFO() << "Creating " << config.pool_settings.initial_size << " mongo connections";
        for (; i < config.pool_settings.initial_size; ++i) {
            auto conn = TryPop(pool_);
            UINVARIANT(conn, "MongoDB initial_size must fit within max_size");
            connections.push_back(std::move(conn));
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << fmt::format(
            "Mongo pool was not fully prepopulated. Expected {} connections, but "
            "{} were created. Error: {}",
            config.pool_settings.initial_size,
            i,
            ex.what()
        );
    }
    for (auto& conn : connections) {
        conn->Push();
    }

    // Must be the last line: UpdateAndListen synchronously calls virtual SetPoolSettings.
    SubscribeToConfig(scopes);
}

CDriverPoolImpl::~CDriverPoolImpl() {
    const tracing::Span span("mongo_destroy");
    pool_.reset();
    generation_cleanup_->Stop();
}

size_t CDriverPoolImpl::InUseApprox() const { return in_use_semaphore_->UsedApprox(); }

size_t CDriverPoolImpl::SizeApprox() const {
    return GetStatistics().pool->current_size.load(std::memory_order_relaxed);
}

size_t CDriverPoolImpl::MaxSize() const { return max_size_.load(); }

const stats::ApmStats& CDriverPoolImpl::GetApmStats() const { return *apm_stats_; }

void CDriverPoolImpl::SetMaxSize(size_t max_size) {
    if (max_size > std::numeric_limits<std::uint32_t>::max()) {
        throw InvalidConfigException("MongoDB max_size exceeds uint32 range for pool '") << Id() << '\'';
    }
    const std::lock_guard lock(settings_mutex_);
    pool_->SetMaxSize(max_size);
    in_use_semaphore_->SetCapacity(max_size);
    max_size_ = max_size;
}

std::string CDriverPoolImpl::DefaultDatabaseName() const { return GetPool()->GetDatabaseName(); }

const std::optional<std::chrono::seconds>& CDriverPoolImpl::GetMaxReplicationLag() const {
    return pool_config_.max_replication_lag;
}

void CDriverPoolImpl::SetPoolSettings(const PoolSettings& pool_settings) {
    pool_settings.Validate(Id());
    const std::lock_guard reload_lock(reload_mutex_);
    ConnPoolPtr replacement;
    if (connecting_limit_ != pool_settings.connecting_limit) {
        const auto current = GetPool();
        const auto uri = MakeUri(Id(), current->GetConnectionString(), pool_config_, pool_settings.connecting_limit);
        if (mongoc_uri_get_option_as_int32(uri.get(), MONGOC_URI_MAXCONNECTING, 2) != current->GetMaxConnecting()) {
            replacement = CreatePool(current->GetConnectionString(), pool_settings.connecting_limit);
        }
    }
    SetMaxSize(pool_settings.max_size);
    {
        const std::lock_guard lock(settings_mutex_);
        if (idle_limit_ != pool_settings.idle_limit) {
            SetNativeIdleLimit(pool_->GetNativePtr(), pool_settings.idle_limit);
            idle_limit_ = pool_settings.idle_limit;
        }
        connecting_limit_ = pool_settings.connecting_limit;
    }
    if (replacement) {
        ReplacePool(std::move(replacement));
    }
}

bool CDriverPoolImpl::IsBulkWriteSupported(const BoundClientPtr& client) const {
    return client.GetPool()->GetBulkWriteSupport() == BulkWriteSupport::kSupported;
}

void CDriverPoolImpl::MarkBulkWriteUnsupported(const BoundClientPtr& client) {
    if (!client.GetPool()->MarkBulkWriteUnsupported()) {
        return;
    }

    LOG_WARNING()
        << "MongoDB server of pool '" << Id()
        << "' does not support the 'bulkWrite' command, MongoDB 8.0 or newer is required. Operations that "
           "are implemented via 'bulkWrite' fall back to the plain commands, max_server_time is ignored "
           "for them";
}

void CDriverPoolImpl::RecheckBulkWriteSupport([[maybe_unused]] const BoundClientPtr& client) {
#ifdef MONGOC_BULKWRITE_H
    if (client.GetPool()->GetBulkWriteSupport() != BulkWriteSupport::kUnknown) {
        return;
    }

    const auto max_wire_version = GetMaxWireVersion(client.get());
    if (!max_wire_version) {
        return;
    }

    const auto support =
        *max_wire_version >= kBulkWriteMinWireVersion ? BulkWriteSupport::kSupported : BulkWriteSupport::kUnsupported;
    if (client.GetPool()->SetBulkWriteSupportIfUnknown(support) && support == BulkWriteSupport::kSupported) {
        LOG_INFO()
            << "MongoDB server of pool '" << Id()
            << "' supports the 'bulkWrite' command, the operations that use it are enabled";
    }
#endif
}

CDriverPoolImpl::ConnPoolPtr CDriverPoolImpl::GetPool() const {
    const std::lock_guard lock(settings_mutex_);
    return pool_;
}

void CDriverPoolImpl::SetConnectionString(const std::string& connection_string) {
    const std::lock_guard reload_lock(reload_mutex_);
    if (GetPool()->GetConnectionString() == connection_string) {
        return;
    }
    ReplacePool(CreatePool(connection_string, connecting_limit_));
    TESTPOINT("mongo-new-connection-string", {});
}

void CDriverPoolImpl::ReplacePool(ConnPoolPtr replacement) {
    ConnPoolPtr retired;
    {
        const std::lock_guard lock(settings_mutex_);
        replacement->SetMaxSize(max_size_.load());
        SetNativeIdleLimit(replacement->GetNativePtr(), idle_limit_.load());
        retired = std::exchange(pool_, std::move(replacement));
    }
    retired->SetMaxSize(kRetiredPoolMaxSize);
    LOG_WARNING() << "New MongoDB pool generation for '" << Id() << "' is active";
}

void CDriverPoolImpl::Ping() {
    static const char* ping_database = "admin";
    static const auto kPingCommand = formats::bson::MakeDoc("ping", 1);
    static const cdriver::ReadPrefsPtr kPingReadPrefs(MONGOC_READ_NEAREST);

    tracing::Span span("mongo_ping");
    span.AddTag(tracing::kDatabaseType, tracing::kDatabaseMongoType);
    span.AddTag(tracing::kDatabaseInstance, ping_database);

    // Do not mess with error stats
    auto conn = Acquire();

    MongoError error;
    stats::OperationStopwatch ping_sw(GetStatistics().pool->ping, "ping");
    const bson_t* native_cmd_bson_ptr = kPingCommand.GetBson().get();
    if (!mongoc_client_command_simple(
            conn.get(),
            ping_database,
            native_cmd_bson_ptr,
            kPingReadPrefs.Get(),
            nullptr,
            error.GetNative()
        ))
    {
        ping_sw.AccountError(error.GetKind());
        error.Throw("Ping failed");
    }

    ping_sw.AccountSuccess();
}

CDriverPoolImpl::BoundClientPtr CDriverPoolImpl::Acquire() {
    const stats::ConnectionWaitStopwatch conn_wait_sw(GetStatistics().pool);
    return BoundClientPtr{Pop(), in_use_semaphore_};
}

CDriverPoolImpl::ConnPtr CDriverPoolImpl::Pop() {
    stats::ConnectionThrottleStopwatch queue_sw(GetStatistics().pool);
    auto queue_deadline = engine::Deadline::FromDuration(pool_config_.queue_timeout);
    std::optional<engine::Deadline::Duration> inherited_timeout;
    CheckTaskCancellation();
    const auto dynamic_config = GetConfig();
    if (dynamic_config[::dynamic_config::USERVER_DEADLINE_PROPAGATION_ENABLED]) {
        HandleCancellations(queue_deadline, inherited_timeout);
    }

    auto in_use_lock = WaitForPermit(*in_use_semaphore_, queue_deadline);
    if (!in_use_lock) {
        ++GetStatistics().pool->overload;
        throw PoolOverloadException("Mongo pool '"
        ) << Id()
          << "' has reached size limit: " << MaxSize() << ". " << MakeQueueDeadlineMessage(inherited_timeout);
    }

    for (;;) {
        auto pool = GetPool();
        auto conn = TryPop(pool);
        queue_sw.Stop();
        if (!conn) {
            if (pool != GetPool() && !queue_deadline.IsReached()) {
                continue;
            }
            ++GetStatistics().pool->overload;
            throw PoolOverloadException("Mongo pool '") << Id() << "' has reached native size limit";
        }
        TESTPOINT("mongo-client-acquired", {});
        CheckTaskCancellation();
        in_use_lock.release();
        return conn;
    }
}

CDriverPoolImpl::ConnPoolPtr CDriverPoolImpl::CreatePool(
    const std::string& connection_string,
    std::size_t connecting_limit
) {
    auto uri = MakeUri(Id(), connection_string, pool_config_, connecting_limit);
    if (!mongoc_uri_get_database(uri.get())) {
        throw InvalidConfigException("MongoDB uri for pool '") << Id() << "' must include database name";
    }
    ConnPoolPtr pool{
        new ConnectionPool(connection_string, std::move(uri), dns_resolver_, apm_stats_, GetStatistics().pool),
        [cleanup = std::weak_ptr{generation_cleanup_}](ConnectionPool* generation) noexcept {
            std::unique_ptr<ConnectionPool> owned{generation};
            if (auto storage = cleanup.lock()) {
                storage->Dispose(std::move(owned));
            }
        }
    };

    {
        mongoc_apm_callbacks_t* cbs = mongoc_apm_callbacks_new();
        mongoc_apm_set_command_succeeded_cb(cbs, CommandSucceeded);
        mongoc_apm_set_command_failed_cb(cbs, CommandFailed);
        mongoc_apm_set_server_heartbeat_started_cb(cbs, HeartbeatStarted);
        mongoc_apm_set_server_heartbeat_succeeded_cb(cbs, HeartbeatSuccess);
        mongoc_apm_set_server_heartbeat_failed_cb(cbs, HeartbeatFailed);
        mongoc_apm_set_topology_changed_cb(cbs, TopologyChanged);
        mongoc_apm_set_topology_opening_cb(cbs, TopologyOpening);
        mongoc_apm_set_topology_closed_cb(cbs, TopologyClosed);
        BSON_ASSERT(mongoc_client_pool_set_apm_callbacks(pool->GetNativePtr(), cbs, pool->GetStatsPtr()));
        mongoc_apm_callbacks_destroy(cbs);
    }

    pool->SetMaxSize(max_size_);
    mongoc_client_pool_set_error_api(pool->GetNativePtr(), MONGOC_ERROR_API_VERSION_2);
    if (utils::impl::kMongoThreadBackendExperiment.IsEnabled()) {
        UINVARIANT(
            mongoc_client_pool_set_stream_initiator(pool->GetNativePtr(), &MakeAsyncStream, &pool->GetInitiatorData()),
            "MongoDB pool stream initiator must be set before creating clients"
        );
    }

    if (!app_name_.empty()) {
        mongoc_client_pool_set_appname(pool->GetNativePtr(), app_name_.c_str());
    }

    return pool;
}

CDriverPoolImpl::ConnPtr CDriverPoolImpl::WrapClient(const ConnPoolPtr& pool, mongoc_client_t* client) {
    UASSERT(client);
    utils::FastScopeGuard return_on_error([&]() noexcept {
        mongoc_client_pool_push(pool->GetNativePtr(), client);
        pool->RefreshStatistics();
    });
    auto conn = std::make_unique<Connection>(pool, client);
    return_on_error.Release();
    return conn;
}

CDriverPoolImpl::ConnPtr CDriverPoolImpl::TryPop(const ConnPoolPtr& pool) {
    auto* client = mongoc_client_pool_try_pop(pool->GetNativePtr());
    pool->RefreshStatistics();
    if (!client) {
        return nullptr;
    }
    auto conn = WrapClient(pool, client);
    return conn;
}

void CDriverPoolImpl::Connection::Push() {
    if (client_) {
        mongoc_client_pool_push(pool_->GetNativePtr(), client_);
        pool_->RefreshStatistics();
        client_ = nullptr;
    }
}

}  // namespace storages::mongo::impl::cdriver_experimental

USERVER_NAMESPACE_END
