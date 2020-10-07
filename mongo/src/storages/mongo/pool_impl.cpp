#include <storages/mongo/pool_impl.hpp>

#include <chrono>
#include <limits>

#include <bson/bson.h>

#include <formats/bson.hpp>
#include <logging/log.hpp>
#include <storages/mongo/exception.hpp>
#include <storages/mongo/mongo_error.hpp>
#include <storages/mongo/stats.hpp>
#include <utils/assert.hpp>
#include <utils/traceful_exception.hpp>

#include <storages/mongo/async_stream.hpp>

namespace storages::mongo::impl {
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

PoolImpl::PoolImpl(std::string id, const std::string& uri_string,
                   const PoolConfig& config)
    : id_(std::move(id)),
      app_name_(config.app_name),
      max_size_(config.max_size),
      idle_limit_(config.idle_limit),
      queue_timeout_(config.queue_timeout),
      size_(0),
      in_use_semaphore_(config.max_size),
      connecting_semaphore_(config.connecting_limit),
      queue_(config.max_size) {
  static const GlobalInitializer kInitMongoc;
  CheckAsyncStreamCompatible();

  uri_ = MakeUri(id_, uri_string, config);
  const char* uri_database = mongoc_uri_get_database(uri_.get());
  if (!uri_database) {
    throw InvalidConfigException("MongoDB uri for pool '")
        << id_ << "' must include database name";
  }
  default_database_ = uri_database;

  ssl_opt_ = MakeSslOpt(uri_.get());

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

PoolImpl::~PoolImpl() {
  maintenance_task_.Stop();

  const ClientDeleter deleter;
  mongoc_client_t* client = nullptr;
  // boost.lockfree pointer magic (FP?)
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  while (queue_.pop(client)) deleter(client);
}

size_t PoolImpl::InUseApprox() const {
  return max_size_ - in_use_semaphore_.RemainingApprox();
}

size_t PoolImpl::SizeApprox() const { return size_.load(); }

size_t PoolImpl::MaxSize() const { return max_size_; }

const std::string& PoolImpl::DefaultDatabaseName() const {
  return default_database_;
}

stats::PoolStatistics& PoolImpl::GetStatistics() { return statistics_; }

PoolImpl::BoundClientPtr PoolImpl::Acquire() {
  stats::ConnectionWaitStopwatch conn_wait_sw(statistics_.pool);
  return {Pop(), ClientPusher(this)};
}

mongoc_client_t* PoolImpl::Pop() {
  stats::ConnectionThrottleStopwatch queue_sw(statistics_.pool);
  const auto queue_deadline = engine::Deadline::FromDuration(queue_timeout_);

  engine::SemaphoreLock in_use_lock(in_use_semaphore_, queue_deadline);
  if (!in_use_lock) {
    ++statistics_.pool->overload;
    throw PoolOverloadException("Mongo pool '")
        << id_ << "' has reached size limit: " << max_size_;
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
        ++statistics_.pool->overload;

        throw PoolOverloadException("Mongo pool '")
            << id_ << "' has too many establishing connections";
      }
      client = Create();
    }
  }

  UASSERT(client);
  in_use_lock.Release();
  return client;
}

void PoolImpl::Push(mongoc_client_t* client) noexcept {
  UASSERT(client);
  if (!queue_.bounded_push(client)) Drop(client);
  in_use_semaphore_.unlock_shared();
}

void PoolImpl::Drop(mongoc_client_t* client) noexcept {
  if (!client) return;

  ClientDeleter()(client);
  --size_;
  ++statistics_.pool->closed;
}

mongoc_client_t* PoolImpl::TryGetIdle() {
  mongoc_client_t* client = nullptr;
  // boost.lockfree pointer magic (FP?)
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  if (queue_.pop(client)) return client;
  return nullptr;
}

mongoc_client_t* PoolImpl::Create() {
  // "admin" is an internal mongodb database and always exists/accessible
  static const char* kPingDatabase = "admin";
  static const auto kPingCommand = formats::bson::MakeDoc("ping", 1);
  static const ReadPrefsPtr kPingReadPrefs(MONGOC_READ_NEAREST);

  LOG_DEBUG() << "Creating mongo connection";

  UnboundClientPtr client(mongoc_client_new_from_uri(uri_.get()));
  mongoc_client_set_error_api(client.get(), MONGOC_ERROR_API_VERSION_2);
  mongoc_client_set_stream_initiator(client.get(), &MakeAsyncStream, &ssl_opt_);

  if (!app_name_.empty()) {
    mongoc_client_set_appname(client.get(), app_name_.c_str());
  }

  // force topology refresh
  // XXX: Periodic task forcing topology refresh?
  MongoError error;
  stats::OperationStopwatch ping_sw(statistics_.pool,
                                    stats::PoolConnectStatistics::kPing);
  if (!mongoc_client_command_simple(
          client.get(), kPingDatabase, kPingCommand.GetBson().get(),
          kPingReadPrefs.Get(), nullptr, error.GetNative())) {
    ping_sw.AccountError(error.GetKind());
    error.Throw("Couldn't create a connection in mongo pool '" + id_ + '\'');
  }

  ++size_;
  ping_sw.AccountSuccess();
  ++statistics_.pool->created;
  return client.release();
}

void PoolImpl::DoMaintenance() {
  LOG_DEBUG() << "Starting mongo pool '" << id_ << "' maintenance";
  for (auto idle_drop_left = kIdleConnectionDropRate;
       idle_drop_left && size_.load() > idle_limit_; --idle_drop_left) {
    LOG_TRACE() << "Trying to drop idle connection";
    Drop(TryGetIdle());
  }
  LOG_DEBUG() << "Finished mongo pool '" << id_ << "' maintenance";
}

}  // namespace storages::mongo::impl
