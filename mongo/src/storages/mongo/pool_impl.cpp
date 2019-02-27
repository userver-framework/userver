#include "pool_impl.hpp"

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/document/value.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/instance.hpp>

#include <logging/log.hpp>
#include <storages/mongo/error.hpp>
#include <storages/mongo/pool.hpp>

#include "logger.hpp"

namespace storages {
namespace mongo {
namespace impl {
namespace {

mongocxx::uri MakeUriWithTimeouts(const std::string& uri, int conn_timeout_ms,
                                  int so_timeout_ms) {
  // mongocxx doesn't provide options setters for uri
  bson_error_t parse_error;
  std::unique_ptr<mongoc_uri_t, decltype(&mongoc_uri_destroy)> parsed_uri(
      mongoc_uri_new_with_error(uri.c_str(), &parse_error),
      &mongoc_uri_destroy);
  if (!parsed_uri) {
    throw InvalidConfig(std::string("bad uri: ") + parse_error.message);
  }
  mongoc_uri_set_option_as_int32(parsed_uri.get(), MONGOC_URI_CONNECTTIMEOUTMS,
                                 conn_timeout_ms);
  mongoc_uri_set_option_as_int32(parsed_uri.get(), MONGOC_URI_SOCKETTIMEOUTMS,
                                 so_timeout_ms);
  return mongocxx::uri(mongoc_uri_get_string(parsed_uri.get()));
}

}  // namespace

PoolImpl::ConnectionToken::ConnectionToken(PoolImpl* pool) : pool_(pool) {
  assert(pool_);

  const auto new_tokens_issued = ++pool_->tokens_issued_;
  if (pool_->tokens_limit_ && new_tokens_issued > pool_->tokens_limit_) {
    --pool_->tokens_issued_;
    throw PoolError("Mongo pool reached pending requests limit: " +
                    std::to_string(new_tokens_issued));
  }
}

PoolImpl::ConnectionToken::~ConnectionToken() {
  if (pool_) --pool_->tokens_issued_;
}

PoolImpl::ConnectionToken::ConnectionToken(ConnectionToken&& other) noexcept
    : pool_(std::exchange(other.pool_, nullptr)) {}

PoolImpl::ConnectionPtr PoolImpl::ConnectionToken::GetConnection() {
  return {pool_->Pop(),
          [pool = pool_](Connection* connection) { pool->Push(connection); }};
}

PoolImpl::PoolImpl(engine::TaskProcessor& task_processor,
                   const std::string& uri, const PoolConfig& config)
    : task_processor_(task_processor),
      queue_(config.max_size),
      size_(0),
      tokens_limit_(config.max_pending_requests),
      tokens_issued_(0) {
  // global mongocxx initializer
  static const mongocxx::instance kDriverInit(std::make_unique<impl::Logger>());

  uri_ = MakeUriWithTimeouts(uri, config.conn_timeout_ms, config.so_timeout_ms);
  if (uri_.hosts().size() != 1) {
    throw InvalidConfig("bad uri: specify exactly one server");
  }
  default_database_name_ = uri_.database();

  try {
    LOG_INFO() << "Creating " << config.min_size << " mongo connections";
    for (size_t i = 0; i < config.min_size; ++i) Push(Create());
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Mongo pool pre-population failed: " << ex;
    Clear();
    throw;
  }
}

PoolImpl::~PoolImpl() { Clear(); }

PoolImpl::ConnectionToken PoolImpl::AcquireToken() {
  return ConnectionToken(this);
}

void PoolImpl::Push(Connection* connection) {
  UASSERT(connection);
  if (queue_.push(connection)) return;
  delete connection;
  --size_;
}

PoolImpl::Connection* PoolImpl::Pop() {
  Connection* connection = nullptr;
  if (queue_.pop(connection)) return connection;
  return Create();
}

const std::string& PoolImpl::GetDefaultDatabaseName() const {
  return default_database_name_;
}

engine::TaskProcessor& PoolImpl::GetTaskProcessor() { return task_processor_; }

PoolImpl::Connection* PoolImpl::Create() {
  namespace bbb = bsoncxx::builder::basic;

  // "admin" is an internal mongodb database and always exists/accessible
  static const std::string kPingDatabase = "admin";
  static const auto kPingCommand = bbb::make_document(bbb::kvp("ping", 1));

  if (size_ > PoolConfig::kCriticalSize) {
    throw PoolError("Mongo pool reached critical size: " +
                    std::to_string(size_));
  }

  LOG_INFO() << "Creating mongo connection, current pool size: "
             << size_.load();

  auto connection = std::make_unique<Connection>(uri_);

  try {
    // force connection
    connection->database(kPingDatabase).run_command(kPingCommand.view());
  } catch (const mongocxx::operation_exception& ex) {
    throw PoolError(std::string("Connection to mongo failed: ") + ex.what());
  }

  ++size_;
  return connection.release();
}

void PoolImpl::Clear() {
  Connection* connection = nullptr;
  while (queue_.pop(connection)) delete connection;
}

}  // namespace impl
}  // namespace mongo
}  // namespace storages
