#include <storages/mongo/cdriver/pool_impl.hpp>

#include <limits>

#include <bson/bson.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <mongoc/mongoc.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/bson.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/traceful_exception.hpp>

#include <storages/mongo/cdriver/async_stream.hpp>
#include <storages/mongo/stats.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

[[maybe_unused]] void MongocCoroFrieldlyUsleep(int64_t usec, void*) noexcept {
    UASSERT(usec >= 0);
    if (engine::current_task::IsTaskProcessorThread()) {
        // we're not sure how it'll behave with interruptible sleeps
        engine::SleepFor(std::chrono::microseconds{usec});
    } else {
        ::usleep(usec);
    }
}

utils::impl::UserverExperiment kServerSelectionTimeoutExperiment{"mongo-server-selection-timeout"};

const std::string kMaintenanceTaskName = "mongo_maintenance";
constexpr size_t kIdleConnectionDropRate = 1;

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

bool HasOption(const UriPtr& uri, const char* opt) {
    const bson_t* options = mongoc_uri_get_options(uri.get());
    bson_iter_t it;
    return bson_iter_init_find_case(&it, options, opt);
}

UriPtr MakeUri(const std::string& pool_id, const std::string& uri_string, const PoolConfig& config) {
    MongoError parse_error;
    UriPtr uri(mongoc_uri_new_with_error(uri_string.c_str(), parse_error.GetNative()));
    if (!uri) {
        throw InvalidConfigException("Bad MongoDB uri for pool '") << pool_id << "': " << parse_error.Message();
    }
    mongoc_uri_set_option_as_int32(
        uri.get(), MONGOC_URI_CONNECTTIMEOUTMS, CheckedDurationMs(config.conn_timeout, MONGOC_URI_CONNECTTIMEOUTMS)
    );
    if (kServerSelectionTimeoutExperiment.IsEnabled()) {
        mongoc_uri_set_option_as_int32(uri.get(), MONGOC_URI_SERVERSELECTIONTIMEOUTMS, 3000);
    }
    mongoc_uri_set_option_as_int32(
        uri.get(), MONGOC_URI_SOCKETTIMEOUTMS, CheckedDurationMs(config.so_timeout, MONGOC_URI_SOCKETTIMEOUTMS)
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
            throw InvalidConfigException("Invalid max replication lag ")
                << max_repl_lag_sec.count() << "s, must be at least " << MONGOC_SMALLEST_MAX_STALENESS_SECONDS << 's';
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
    if (!inherited_timeout) return {};
    return fmt::format(
        "Queue timeout set by deadline propagation: {}. ",
        std::chrono::duration_cast<std::chrono::milliseconds>(*inherited_timeout)
    );
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

    if (engine::current_task::ShouldCancel()) {
        throw CancelledException("Operation cancelled: ") << ToString(engine::current_task::CancellationReason());
    }
}

stats::ConnStats& GetStats(void* stats_ptr) {
    UASSERT(stats_ptr);
    return *reinterpret_cast<stats::ConnStats*>(stats_ptr);
}

void CommandSuccessed(const mongoc_apm_command_succeeded_t* event) {
    auto& stats = GetStats(mongoc_apm_command_succeeded_get_context(event));
    stats.event_stats_.sucess += utils::statistics::Rate{1};
}

void CommandFailed(const mongoc_apm_command_failed_t* event) {
    auto& stats = GetStats(mongoc_apm_command_failed_get_context(event));
    stats.event_stats_.failed += utils::statistics::Rate{1};
}

void HearbeatStarted(const mongoc_apm_server_heartbeat_started_t* event) {
    auto& stats = GetStats(mongoc_apm_server_heartbeat_started_get_context(event));
    ++stats.apm_stats_->heartbeats.start;
    LOG_LIMITED_DEBUG() << mongoc_apm_server_heartbeat_started_get_host(event)->host_and_port << " heartbeat started";
}

void HeartbeatSuccess(const mongoc_apm_server_heartbeat_succeeded_t* event) {
    auto& stats = GetStats(mongoc_apm_server_heartbeat_succeeded_get_context(event));
    ++stats.apm_stats_->heartbeats.success;
    LOG_LIMITED_DEBUG() << mongoc_apm_server_heartbeat_succeeded_get_host(event)->host_and_port
                        << " heartbeat succeeded";
}

void HearbeatFailed(const mongoc_apm_server_heartbeat_failed_t* event) {
    auto& stats = GetStats(mongoc_apm_server_heartbeat_failed_get_context(event));
    ++stats.apm_stats_->heartbeats.failed;
    LOG_LIMITED_WARNING() << mongoc_apm_server_heartbeat_failed_get_host(event)->host_and_port << " heartbeat failed";
}

std::string CreateTopologyChangeMessage(const mongoc_apm_topology_changed_t* event) {
    const mongoc_topology_description_t* prev_td = mongoc_apm_topology_changed_get_previous_description(event);
    const mongoc_topology_description_t* new_td = mongoc_apm_topology_changed_get_new_description(event);
    std::size_t nprev_server_desc{0};
    mongoc_server_description_t** prev_sds = mongoc_topology_description_get_servers(prev_td, &nprev_server_desc);
    std::size_t nnew_server_desc{0};
    mongoc_server_description_t** new_sds = mongoc_topology_description_get_servers(new_td, &nnew_server_desc);

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

    mongoc_read_prefs_destroy(prefs);
    mongoc_server_descriptions_destroy_all(prev_sds, nprev_server_desc);
    mongoc_server_descriptions_destroy_all(new_sds, nnew_server_desc);

    return topology_msg;
}

void TopologyChanged(const mongoc_apm_topology_changed_t* event) {
    auto& stats = GetStats(mongoc_apm_topology_changed_get_context(event));
    ++stats.apm_stats_->topology.changed;

    LOG_INFO() << CreateTopologyChangeMessage(event);
}

void TopologyOpening(const mongoc_apm_topology_opening_t*) {
    LOG_DEBUG() << "The driver initializes a mongoc_topology_description_t";
}

void TopologyClosed(const mongoc_apm_topology_closed_t*) {
    LOG_DEBUG() << "The driver stops monitoring a server topology and destroys it";
}

}  // namespace

CDriverPoolImpl::CDriverPoolImpl(
    std::string id,
    const std::string& uri_string,
    const PoolConfig& config,
    clients::dns::Resolver* dns_resolver,
    dynamic_config::Source config_source
)
    : PoolImpl(std::move(id), config, config_source),
      app_name_(config.app_name),
      init_data_{dns_resolver, {}},
      max_size_(config.pool_settings.max_size),
      idle_limit_(config.pool_settings.idle_limit),
      queue_timeout_(config.queue_timeout),
      size_(0),
      in_use_semaphore_(config.pool_settings.max_size),
      connecting_semaphore_(config.pool_settings.connecting_limit),
      // FP?: pointer magic in boost.lockfree
      // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
      queue_(config.pool_settings.max_size) {
    static const GlobalInitializer kInitMongoc;
    GlobalInitializer::LogInitWarningsOnce();

    uri_ = MakeUri(Id(), uri_string, config);
    const char* uri_database = mongoc_uri_get_database(uri_.get());
    if (!uri_database) {
        throw InvalidConfigException("MongoDB uri for pool '") << Id() << "' must include database name";
    }
    default_database_ = uri_database;

    init_data_.ssl_opt = MakeSslOpt(uri_.get());

    std::size_t i = 0;
    try {
        tracing::Span span("mongo_prepopulate");
        LOG_INFO() << "Creating " << config.pool_settings.initial_size << " mongo connections";
        for (; i < config.pool_settings.initial_size; ++i) {
            engine::SemaphoreLock lock(in_use_semaphore_);
            Push(Create());
            lock.Release();
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

    maintenance_task_.Start(
        kMaintenanceTaskName,
        {config.maintenance_period, {utils::PeriodicTask::Flags::kStrong}},
        [this] { DoMaintenance(); }
    );
}

CDriverPoolImpl::~CDriverPoolImpl() {
    tracing::Span span("mongo_destroy");
    maintenance_task_.Stop();
}

size_t CDriverPoolImpl::InUseApprox() const {
    auto max_size = max_size_.load();
    auto remaining = in_use_semaphore_.RemainingApprox();
    if (max_size < remaining) {
        // Possible during max_size_ change
        max_size = remaining;
    }
    return max_size - remaining;
}

size_t CDriverPoolImpl::SizeApprox() const { return size_.load(); }

size_t CDriverPoolImpl::MaxSize() const { return max_size_.load(); }

const stats::ApmStats& CDriverPoolImpl::GetApmStats() const { return apm_stats_; }

void CDriverPoolImpl::SetMaxSize(size_t max_size) { max_size_ = max_size; }

const std::string& CDriverPoolImpl::DefaultDatabaseName() const { return default_database_; }

void CDriverPoolImpl::SetPoolSettings(const PoolSettings& pool_settings) {
    SetMaxSize(pool_settings.max_size);
    idle_limit_ = pool_settings.idle_limit;
    in_use_semaphore_.SetCapacity(pool_settings.max_size);
    connecting_semaphore_.SetCapacity(pool_settings.connecting_limit);
}

void CDriverPoolImpl::Ping() {
    static const char* kPingDatabase = "admin";
    static const auto kPingCommand = formats::bson::MakeDoc("ping", 1);
    static const ReadPrefsPtr kPingReadPrefs(MONGOC_READ_NEAREST);

    tracing::Span span("mongo_ping");
    span.AddTag(tracing::kDatabaseType, tracing::kDatabaseMongoType);
    span.AddTag(tracing::kDatabaseInstance, kPingDatabase);

    // Do not mess with error stats
    auto conn = Acquire();

    MongoError error;
    stats::OperationStopwatch ping_sw(GetStatistics().pool->ping, "ping");
    const bson_t* native_cmd_bson_ptr = kPingCommand.GetBson().get();
    if (!mongoc_client_command_simple(
            conn.get(), kPingDatabase, native_cmd_bson_ptr, kPingReadPrefs.Get(), nullptr, error.GetNative()
        )) {
        ping_sw.AccountError(error.GetKind());
        error.Throw("Ping failed");
    }

    ping_sw.AccountSuccess();
}

CDriverPoolImpl::BoundClientPtr CDriverPoolImpl::Acquire() {
    const stats::ConnectionWaitStopwatch conn_wait_sw(GetStatistics().pool);
    return BoundClientPtr{Pop(), this};
}

CDriverPoolImpl::ConnPtr CDriverPoolImpl::Pop() {
    stats::ConnectionThrottleStopwatch queue_sw(GetStatistics().pool);

    auto queue_deadline = engine::Deadline::FromDuration(queue_timeout_);
    std::optional<engine::Deadline::Duration> inherited_timeout{};

    const auto dynamic_config = GetConfig();
    if (dynamic_config[kDeadlinePropagationEnabled]) {
        HandleCancellations(queue_deadline, inherited_timeout);
    }

    engine::SemaphoreLock in_use_lock(in_use_semaphore_, queue_deadline);
    if (!in_use_lock) {
        ++GetStatistics().pool->overload;
        throw PoolOverloadException("Mongo pool '") << Id() << "' has reached size limit: " << max_size_.load() << ". "
                                                    << MakeQueueDeadlineMessage(inherited_timeout);
    }

    auto conn = TryGetIdle();
    if (!conn) {
        const engine::SemaphoreLock connecting_lock(connecting_semaphore_, queue_deadline);
        queue_sw.Stop();

        // retry getting idle connection after the wait
        conn = TryGetIdle();
        if (!conn) {
            if (!connecting_lock) {
                ++GetStatistics().pool->overload;
                throw PoolOverloadException("Mongo pool '") << Id() << "' has too many establishing connections. "
                                                            << MakeQueueDeadlineMessage(inherited_timeout);
            }
            conn = Create();
        }
    }

    UASSERT(conn);
    in_use_lock.Release();
    return conn;
}

void CDriverPoolImpl::Push(ConnPtr conn) noexcept {
    UASSERT(conn);
    if (SizeApprox() > MaxSize()) {
        /*
         * We might get many connections, and after that make the max_size lower
         * (e.g. due to congestion control). In this case extra connections must be
         * dropped.
         */
        Drop(std::move(conn));
        UASSERT(!conn);
    }
    if (conn && !queue_.enqueue(std::move(conn))) {
        --size_;
        ++GetStatistics().pool->closed;
    }

    in_use_semaphore_.unlock_shared();
}

void CDriverPoolImpl::Drop(ConnPtr conn) noexcept {
    if (!conn) return;

    --size_;
    ++GetStatistics().pool->closed;
}

CDriverPoolImpl::ConnPtr CDriverPoolImpl::TryGetIdle() {
    ConnPtr conn{};
    if (queue_.try_dequeue(conn)) return conn;
    return nullptr;
}

CDriverPoolImpl::ConnPtr CDriverPoolImpl::Create() {
    // "admin" is an internal mongodb database and always exists/accessible
    static const char* kPingDatabase = "admin";
    static const auto kPingCommand = formats::bson::MakeDoc("ping", 1);
    static const ReadPrefsPtr kPingReadPrefs(MONGOC_READ_NEAREST);

    LOG_DEBUG() << "Creating mongo connection";

    ConnPtr conn = std::make_unique<Connection>(mongoc_client_new_from_uri(uri_.get()), &apm_stats_);

    // Set command monitoring events to get command durations.
    {
        mongoc_apm_callbacks_t* cbs = mongoc_apm_callbacks_new();
        mongoc_apm_set_command_succeeded_cb(cbs, CommandSuccessed);
        mongoc_apm_set_command_failed_cb(cbs, CommandFailed);
        mongoc_apm_set_server_heartbeat_started_cb(cbs, HearbeatStarted);
        mongoc_apm_set_server_heartbeat_succeeded_cb(cbs, HeartbeatSuccess);
        mongoc_apm_set_server_heartbeat_failed_cb(cbs, HearbeatFailed);
        mongoc_apm_set_topology_changed_cb(cbs, TopologyChanged);
        mongoc_apm_set_topology_opening_cb(cbs, TopologyOpening);
        mongoc_apm_set_topology_closed_cb(cbs, TopologyClosed);
        BSON_ASSERT(mongoc_client_set_apm_callbacks(conn->GetNativePtr(), cbs, conn->GetStatsPtr()));
        mongoc_apm_callbacks_destroy(cbs);
    }

    mongoc_client_set_error_api(conn->GetNativePtr(), MONGOC_ERROR_API_VERSION_2);
    mongoc_client_set_stream_initiator(conn->GetNativePtr(), &MakeAsyncStream, &init_data_);
#if MONGOC_CHECK_VERSION(1, 26, 0)
    mongoc_client_set_usleep_impl(conn->GetNativePtr(), &MongocCoroFrieldlyUsleep, nullptr);
#else
    LOG_LIMITED_WARNING() << "Cannot use coro-friendly usleep in mongo driver, "
                             "link against newer mongo-c-driver to fix";
#endif

    if (!app_name_.empty()) {
        mongoc_client_set_appname(conn->GetNativePtr(), app_name_.c_str());
    }

    // force topology refresh
    // XXX: Periodic task forcing topology refresh?
    MongoError error;
    stats::OperationStopwatch ping_sw(GetStatistics().pool->ping, "ping");
    const bson_t* native_cmd_bson_ptr = kPingCommand.GetBson().get();
    if (!mongoc_client_command_simple(
            conn->GetNativePtr(), kPingDatabase, native_cmd_bson_ptr, kPingReadPrefs.Get(), nullptr, error.GetNative()
        )) {
        ping_sw.AccountError(error.GetKind());
        error.Throw("Couldn't create a connection in mongo pool '" + Id() + '\'');
    }

    ++size_;
    ping_sw.AccountSuccess();
    ++GetStatistics().pool->created;
    return conn;
}

void CDriverPoolImpl::DoMaintenance() {
    LOG_DEBUG() << "Starting mongo pool '" << Id() << "' maintenance";
    for (auto idle_drop_left = kIdleConnectionDropRate; idle_drop_left && size_.load() > idle_limit_;
         --idle_drop_left) {
        LOG_TRACE() << "Trying to drop idle connection";
        Drop(TryGetIdle());
    }
    LOG_DEBUG() << "Finished mongo pool '" << Id() << "' maintenance";
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
