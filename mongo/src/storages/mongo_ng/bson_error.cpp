#include <storages/mongo_ng/bson_error.hpp>

#include <storages/mongo_ng/exception.hpp>

namespace storages::mongo_ng::impl {

void BsonError::Throw() const {
  // TODO
  throw MongoException(value_.message);
}

}  // namespace storages::mongo_ng::impl
