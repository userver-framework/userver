#include <storages/mongo_ng/bson_error.hpp>

#include <boost/stacktrace/stacktrace.hpp>

#include <logging/stacktrace_cache.hpp>
#include <storages/mongo_ng/exception.hpp>

namespace storages::mongo_ng::impl {

void BsonError::Throw() const {
  // TODO
  throw MongoException(
      value_.message + std::string(", stacktrace:\n") +
      logging::stacktrace_cache::to_string(boost::stacktrace::stacktrace{}));
}

}  // namespace storages::mongo_ng::impl
