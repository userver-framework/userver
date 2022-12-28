#include <storages/mongo/cdriver/pool_impl.hpp>

#include <limits>

#include <bson/bson.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/formats/bson.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/traceful_exception.hpp>

#include <storages/mongo/cdriver/async_stream.hpp>
#include <storages/mongo/stats.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

const std::string kMaintenanceTaskName = "mongo_maintenance";
constexpr size_t kIdleConnectionDropRate = 1;

int32_t CheckedDurationMs(const std::chrono::milliseconds& timeout,
                          const char* name) {
  auto timeout_ms = timeout.count();
  if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int32_t>::max()) {
    throw InvalidConfigException("Bad value ")
        << timeout_ms << "ms for '" << name << '\'';
  }
  return timeout_ms;
}

int32_t CheckedDurationSeconds(const std::chrono::seconds& timeout,
                               const char* name) {
  auto timeout_sec = timeout.count();
  if (timeout_sec < 0 || timeout_sec > std::numeric_limits<int32_t>::max()) {
    throw InvalidConfigException("Bad value ")
        << timeout_sec << "s for '" << name << '\'';
  }
  return timeout_sec;
}

bool HasOption(const UriPtr& uri, const char* opt) {
  const bson_t* options = mongoc_uri_get_options(uri.get());
  bson_iter_t it;
  return bson_iter_init_find_case(&it, options, opt);
}

UriPtr MakeUri(const std::string& pool_id, const std::string& uri_string,
               const PoolConfig& config) {
  MongoError parse_error;
  UriPtr uri(
      mongoc_uri_new_with_error(uri_string.c_str(), parse_error.GetNative()));
  if (!uri) {
    throw InvalidConfigException("Bad MongoDB uri for pool '")
        << pool_id << "': " << parse_error.Message();
  }
  mongoc_uri_set_option_as_int32(
      uri.get(), MONGOC_URI_CONNECTTIMEOUTMS,
      CheckedDurationMs(config.conn_timeout, MONGOC_URI_CONNECTTIMEOUTMS));
  mongoc_uri_set_option_as_int32(
      uri.get(), MONGOC_URI_SOCKETTIMEOUTMS,
      CheckedDurationMs(config.so_timeout, MONGOC_URI_SOCKETTIMEOUTMS));
  if (config.local_threshold) {
    mongoc_uri_set_option_as_int32(
        uri.get(), MONGOC_URI_LOCALTHRESHOLDMS,
        CheckedDurationMs(*config.local_threshold,
                          MONGOC_URI_LOCALTHRESHOLDMS));
  }
  if (config.max_replication_lag) {
    const auto max_repl_lag_sec =
        std::chrono::duration_cast<std::chrono::seconds>(
            *config.max_replication_lag);
    if (max_repl_lag_sec.count() < MONGOC_SMALLEST_MAX_STALENESS_SECONDS) {
      throw InvalidConfigException("Invalid max replication lag ")
          << max_repl_lag_sec.count() << "s, must be at least "
          << MONGOC_SMALLEST_MAX_STALENESS_SECONDS << 's';
    }
    mongoc_uri_set_option_as_int32(
        uri.get(), MONGOC_URI_MAXSTALENESSSECONDS,
        CheckedDurationSeconds(max_repl_lag_sec,
                               MONGOC_URI_MAXSTALENESSSECONDS));
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
  ssl_opt.pem_file = mongoc_uri_get_option_as_utf8(
      uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, nullptr);
  ssl_opt.pem_pwd = mongoc_uri_get_option_as_utf8(
      uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, nullptr);
  ssl_opt.ca_file =
      mongoc_uri_get_option_as_utf8(uri, MONGOC_URI_TLSCAFILE, nullptr);
  ssl_opt.weak_cert_validation = mongoc_uri_get_option_as_bool(
      uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false);
  ssl_opt.allow_invalid_hostname = mongoc_uri_get_option_as_bool(
      uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false);
  return ssl_opt;
}

}  // namespace

CDriverPoolImpl::CDriverPoolImpl(std::string id, const std::string& uri_string,
                                 const PoolConfig& config,
                                 clients::dns::Resolver* dns_resolver,
                                 dynamic_config::Source config_source)
    : PoolImpl(std::move(id), config_source),
      app_name_(config.app_name),
      init_data_{dns_resolver, {}},
      max_size_(config.max_size),
      idle_limit_(config.idle_limit),
      queue_timeout_(config.queue_timeout),
      size_(0),
      in_use_semaphore_(config.max_size),
      connecting_semaphore_(config.connecting_limit),
      // FP?: pointer magic in boost.lockfree
      // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
      queue_(config.max_size) {
  static const GlobalInitializer kInitMongoc;
  GlobalInitializer::LogInitWarningsOnce();
  CheckAsyncStreamCompatible();

  uri_ = MakeUri(Id(), uri_string, config);
  const char* uri_database = mongoc_uri_get_database(uri_.get());
  if (!uri_database) {
    throw InvalidConfigException("MongoDB uri for pool '")
        << Id() << "' must include database name";
  }
  default_database_ = uri_database;

  init_data_.ssl_opt = MakeSslOpt(uri_.get());

  try {
    LOG_INFO() << "Creating " << config.initial_size << " mongo connections";
    for (size_t i = 0; i < config.initial_size; ++i) {
      engine::SemaphoreLock lock(in_use_semaphore_);
      Push(Create());
      lock.Release();
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Mongo pool was not fully prepopulated: " << ex;
  }

  maintenance_task_.Start(kMaintenanceTaskName,
                          {config.maintenance_period,
                           {utils::PeriodicTask::Flags::kStrong,
                            utils::PeriodicTask::Flags::kCritical}},
                          [this] { DoMaintenance(); });
}

CDriverPoolImpl::~CDriverPoolImpl() {
  maintenance_task_.Stop();

  const ClientDeleter deleter;
  mongoc_client_t* client = nullptr;
  while (queue_.pop(client)) deleter(client);
}

size_t CDriverPoolImpl::InUseApprox() const {
  return max_size_ - in_use_semaphore_.RemainingApprox();
}

size_t CDriverPoolImpl::SizeApprox() const { return size_.load(); }

size_t CDriverPoolImpl::MaxSize() const { return max_size_; }

const std::string& CDriverPoolImpl::DefaultDatabaseName() const {
  return default_database_;
}

CDriverPoolImpl::BoundClientPtr CDriverPoolImpl::Acquire() {
  stats::ConnectionWaitStopwatch conn_wait_sw(GetStatistics().pool);
  return {Pop(), ClientPusher(this)};
}

mongoc_client_t* CDriverPoolImpl::Pop() {
  stats::ConnectionThrottleStopwatch queue_sw(GetStatistics().pool);
  const auto queue_deadline = engine::Deadline::FromDuration(queue_timeout_);

  engine::SemaphoreLock in_use_lock(in_use_semaphore_, queue_deadline);
  if (!in_use_lock) {
    ++GetStatistics().pool->overload;
    throw PoolOverloadException("Mongo pool '")
        << Id() << "' has reached size limit: " << max_size_;
  }

  auto* client = TryGetIdle();
  if (!client) {
    engine::SemaphoreLock connecting_lock(connecting_semaphore_,
                                          queue_deadline);
    queue_sw.Stop();

    // retry getting idle connection after the wait
    client = TryGetIdle();
    if (!client) {
      if (!connecting_lock) {
        ++GetStatistics().pool->overload;

        throw PoolOverloadException("Mongo pool '")
            << Id() << "' has too many establishing connections";
      }
      client = Create();
    }
  }

  UASSERT(client);
  in_use_lock.Release();
  return client;
}

void CDriverPoolImpl::Push(mongoc_client_t* client) noexcept {
  UASSERT(client);
  if (!queue_.bounded_push(client)) Drop(client);
  in_use_semaphore_.unlock_shared();
}

void CDriverPoolImpl::Drop(mongoc_client_t* client) noexcept {
  if (!client) return;

  ClientDeleter()(client);
  --size_;
  ++GetStatistics().pool->closed;
}

mongoc_client_t* CDriverPoolImpl::TryGetIdle() {
  mongoc_client_t* client = nullptr;
  if (queue_.pop(client)) return client;
  return nullptr;
}

mongoc_client_t* CDriverPoolImpl::Create() {
  // "admin" is an internal mongodb database and always exists/accessible
  static const char* kPingDatabase = "admin";
  static const auto kPingCommand = formats::bson::MakeDoc("ping", 1);
  static const ReadPrefsPtr kPingReadPrefs(MONGOC_READ_NEAREST);

  LOG_DEBUG() << "Creating mongo connection";

  UnboundClientPtr client(mongoc_client_new_from_uri(uri_.get()));
  mongoc_client_set_error_api(client.get(), MONGOC_ERROR_API_VERSION_2);
  mongoc_client_set_stream_initiator(client.get(), &MakeAsyncStream,
                                     &init_data_);

  if (!app_name_.empty()) {
    mongoc_client_set_appname(client.get(), app_name_.c_str());
  }

  // force topology refresh
  // XXX: Periodic task forcing topology refresh?
  MongoError error;
  stats::OperationStopwatch ping_sw(GetStatistics().pool,
                                    stats::PoolConnectStatistics::kPing);
  const bson_t* native_cmd_bson_ptr = kPingCommand.GetBson().get();
  if (!mongoc_client_command_simple(client.get(), kPingDatabase,
                                    native_cmd_bson_ptr, kPingReadPrefs.Get(),
                                    nullptr, error.GetNative())) {
    ping_sw.AccountError(error.GetKind());
    error.Throw("Couldn't create a connection in mongo pool '" + Id() + '\'');
  }

  ++size_;
  ping_sw.AccountSuccess();
  ++GetStatistics().pool->created;
  return client.release();
}

void CDriverPoolImpl::DoMaintenance() {
  LOG_DEBUG() << "Starting mongo pool '" << Id() << "' maintenance";
  for (auto idle_drop_left = kIdleConnectionDropRate;
       idle_drop_left && size_.load() > idle_limit_; --idle_drop_left) {
    LOG_TRACE() << "Trying to drop idle connection";
    Drop(TryGetIdle());
  }
  LOG_DEBUG() << "Finished mongo pool '" << Id() << "' maintenance";
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
