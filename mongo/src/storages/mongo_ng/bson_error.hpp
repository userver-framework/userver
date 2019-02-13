#pragma once

#include <bson/bson.h>

namespace storages::mongo_ng::impl {

class BsonError {
 public:
  BsonError() { value_.code = 0; }

  explicit operator bool() const { return !!value_.code; }

  const bson_error_t& operator*() const { return value_; }
  const bson_error_t* operator->() const { return &value_; }

  bson_error_t* Get() { return &value_; }

  /// Unconditionally throws specialized MongoException based on value
  void Throw() const;

 private:
  bson_error_t value_;
};

}  // namespace storages::mongo_ng::impl
