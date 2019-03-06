#pragma once

#include <string>

#include <bson/bson.h>

namespace storages::mongo_ng {

class MongoError {
 public:
  /// @cond
  /// Creates an unspecified error
  MongoError();
  /// @endcond

  /// Checks whether an error is set up
  explicit operator bool() const;

  /// Checks whether this is a server error
  bool IsServerError() const;

  uint32_t Code() const;
  const char* Message() const;

  /// @cond
  /// Error domain, for internal use
  uint32_t Domain() const;

  /// Returns native type, internal use only
  bson_error_t* GetNative();
  /// @endcond

  /// Unconditionally throws specialized MongoException
  [[noreturn]] void Throw(std::string context) const;

 private:
  bson_error_t value_;
};

}  // namespace storages::mongo_ng
