#pragma once

#include <string>

#include <bson/bson.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

/// MongoDB error
class MongoError {
 public:
  /// Error kinds
  enum class Kind {
    kNoError,               ///< Error was not reported correctly
    kNetwork,               ///< @copybrief NetworkException
    kClusterUnavailable,    ///< @copybrief ClusterUnavailableException
    kIncompatibleServer,    ///< @copybrief IncompatibleServerException
    kAuthentication,        ///< @copybrief AuthenticationException
    kQuery,                 ///< @copybrief QueryException
    kInvalidQueryArgument,  ///< @copybrief InvalidQueryArgumentException
    kServer,                ///< @copybrief ServerException
    kWriteConcern,          ///< @copybrief WriteConcernException
    kDuplicateKey,          ///< @copybrief DuplicateKeyException
    kOther                  ///< Unclassified error
  };

  /// @cond
  /// Creates a not-an-error
  MongoError();
  /// @endcond

  /// Checks whether an error is set up
  explicit operator bool() const;

  /// Checks whether this is a server error
  bool IsServerError() const;
  Kind GetKind() const;

  uint32_t Code() const;
  const char* Message() const;

  /// @cond
  /// Error domain, for internal use
  uint32_t Domain() const;

  /// Returns native type, internal use only
  bson_error_t* GetNative();
  /// @endcond

  /// Unconditionally throws specialized MongoException
  [[noreturn]] void Throw(std::string prefix) const;

 private:
  bson_error_t value_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
