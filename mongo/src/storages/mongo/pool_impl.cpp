#include <storages/mongo/pool_impl.hpp>

#include <chrono>
#include <limits>

#include <formats/bson.hpp>
#include <logging/log.hpp>
#include <storages/mongo/exception.hpp>
#include <storages/mongo/mongo_error.hpp>
#include <utils/assert.hpp>
#include <utils/traceful_exception.hpp>

#include <storages/mongo/async_stream.hpp>

namespace storages::mongo::impl {
namespace {

int32_t CheckedTimeoutMs(const std::chrono::milliseconds& timeout,
                         const char* name) {
  auto timeout_ms = timeout.count();
  if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int32_t>::max()) {
    throw InvalidConfigException("Bad value ")
        << timeout_ms << "ms for '" << name << '\'';
  }
  return timeout_ms;
}

UriPtr MakeUri(const std::string& pool_id, const std::string& uri_string,
               std::chrono::milliseconds conn_timeout,
               std::chrono::milliseconds so_timeout) {
  MongoError parse_error;
  UriPtr uri(
      mongoc_uri_new_with_error(uri_string.c_str(), parse_error.GetNative()));
  if (!uri) {
    throw InvalidConfigException("Bad MongoDB uri for pool '")
        << pool_id << "': " << parse_error.Message();
  }
  mongoc_uri_set_option_as_int32(
      uri.get(), MONGOC_URI_CONNECTTIMEOUTMS,
      CheckedTimeoutMs(conn_timeout, MONGOC_URI_CONNECTTIMEOUTMS));
  mongoc_uri_set_option_as_int32(
      uri.get(), MONGOC_URI_SOCKETTIMEOUTMS,
      CheckedTimeoutMs(so_timeout, MONGOC_URI_SOCKETTIMEOUTMS));
  return uri;
}

mongoc_ssl_opt_t MakeSslOpt(const mongoc_uri_t* uri) {
  mongoc_ssl_opt_t ssl_opt = *mongoc_ssl_opt_get_default();
  ssl_opt.pem_file = mongoc_uri_get_option_as_utf8(
      uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, nullptr);
  ssl_opt.pem_pwd = mongoc_uri_get_option_as_utf8(
      uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, nullptr);
  ssl_opt.ca_file = mongoc_uri_get_option_as_utf8(
      uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, nullptr);
  ssl_opt.weak_cert_validation = mongoc_uri_get_option_as_bool(
      uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false);
  ssl_opt.allow_invalid_hostname = mongoc_uri_get_option_as_bool(
      uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false);
  return ssl_opt;
}

}  // namespace

PoolImpl::PoolImpl(std::string id, const std::string& uri_string,
                   const PoolConfig& config)
    : id_(std::move(id)),
      app_name_(config.app_name),
      max_size_(config.max_size),
      queue_timeout_(config.queue_timeout),
      size_semaphore_(config.max_size),
      connecting_semaphore_(config.connecting_limit),
      queue_(config.idle_limit) {
  static const GlobalInitializer kInitMongoc;
  CheckAsyncStreamCompatible();

  uri_ = MakeUri(id_, uri_string, config.conn_timeout, config.so_timeout);
  const char* uri_database = mongoc_uri_get_database(uri_.get());
  if (!uri_database) {
    throw InvalidConfigException("MongoDB uri for pool '")
        << id_ << "' must include database name";
  }
  default_database_ = uri_database;

  ssl_opt_ = MakeSslOpt(uri_.get());

  try {
    LOG_INFO() << "Creating " << config.initial_size << " mongo connections";
    for (size_t i = 0; i < config.initial_size; ++i) Push(Create());
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Mongo pool was not fully prepopulated: " << ex;
  }
}

PoolImpl::~PoolImpl() {
  const ClientDeleter deleter;
  mongoc_client_t* client = nullptr;
  while (queue_.pop(client)) deleter(client);
}

const std::string& PoolImpl::DefaultDatabaseName() const {
  return default_database_;
}

PoolImpl::BoundClientPtr PoolImpl::Acquire() {
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  return {Pop(), ClientPusher(this)};
}

void PoolImpl::Push(mongoc_client_t* client) noexcept {
  UASSERT(client);
  if (queue_.bounded_push(client)) return;
  ClientDeleter()(client);
  size_semaphore_.unlock_shared();
}

mongoc_client_t* PoolImpl::Pop() {
  if (auto* client = TryGetIdle()) return client;
  return Create();
}

mongoc_client_t* PoolImpl::TryGetIdle() {
  mongoc_client_t* client = nullptr;
  if (queue_.pop(client)) return client;
  return nullptr;
}

mongoc_client_t* PoolImpl::Create() {
  // "admin" is an internal mongodb database and always exists/accessible
  static const char* kPingDatabase = "admin";
  static const auto kPingCommand = formats::bson::MakeDoc("ping", 1);
  static const ReadPrefsPtr kPingReadPrefs(
      mongoc_read_prefs_new(MONGOC_READ_NEAREST));

  auto deadline = engine::Deadline::FromDuration(queue_timeout_);

  if (!size_semaphore_.try_lock_shared_until(deadline)) {
    // retry getting idle connection after the wait
    if (auto* client = TryGetIdle()) return client;

    throw MongoException("Mongo pool '")
        << id_ << "' has reached size limit: " << max_size_;
  }
  std::shared_lock<engine::Semaphore> size_lock(size_semaphore_,
                                                std::adopt_lock);

  if (!connecting_semaphore_.try_lock_shared_until(deadline)) {
    // retry getting idle connection after the wait
    if (auto* client = TryGetIdle()) return client;

    throw MongoException("Mongo pool '")
        << id_ << "' has too many establishing connections";
  }
  std::shared_lock<engine::Semaphore> connecting_lock(connecting_semaphore_,
                                                      std::adopt_lock);

  // retry getting idle connection after the wait
  if (auto* client = TryGetIdle()) return client;

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
  if (!mongoc_client_command_simple(
          client.get(), kPingDatabase, kPingCommand.GetBson().get(),
          kPingReadPrefs.get(), nullptr, error.GetNative())) {
    error.Throw("Couldn't create a connection in mongo pool '" + id_ + '\'');
  }

  size_lock.release();
  return client.release();
}

}  // namespace storages::mongo::impl
