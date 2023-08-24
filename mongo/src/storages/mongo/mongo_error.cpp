#include <userver/storages/mongo/mongo_error.hpp>

#include <mongoc/mongoc.h>

#include <userver/storages/mongo/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

MongoError::MongoError() : value_{0, 0, {'\0'}} {}

MongoError::operator bool() const { return !!value_.code; }

// NOTE: must be manually synchronized with GetKind
bool MongoError::IsServerError() const {
  switch (value_.domain) {
    case MONGOC_ERROR_WRITE_CONCERN:
      return true;

    case MONGOC_ERROR_SERVER:
      return value_.code != MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND;

    default:
      return false;
  }
}

// NOTE: must be manually synchronized with IsServerError
MongoError::Kind MongoError::GetKind() const {
  switch (value_.domain) {
    case 0:
      return Kind::kNoError;

    case MONGOC_ERROR_CLIENT:
      switch (value_.code) {
        case MONGOC_ERROR_CLIENT_GETNONCE:
        case MONGOC_ERROR_CLIENT_AUTHENTICATE:
          return Kind::kAuthentication;

        case MONGOC_ERROR_STREAM_CONNECT:
          return Kind::kNetwork;

        default:;  // return kOther
      }
      break;

    case MONGOC_ERROR_STREAM:
      switch (value_.code) {
        case MONGOC_ERROR_STREAM_NAME_RESOLUTION:
        case MONGOC_ERROR_STREAM_SOCKET:
        case MONGOC_ERROR_STREAM_CONNECT:
          return Kind::kNetwork;

        default:;  // return kOther
      }
      break;

    case MONGOC_ERROR_PROTOCOL:
      switch (value_.code) {
        case MONGOC_ERROR_PROTOCOL_INVALID_REPLY:
          return Kind::kNetwork;

        case MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION:
          return Kind::kIncompatibleServer;

        default:;  // return kOther
      }
      break;

    case MONGOC_ERROR_CURSOR:
      switch (value_.code) {
        case MONGOC_ERROR_CURSOR_INVALID_CURSOR:
          return Kind::kQuery;

        default:;  // return kOther
      }
      break;

    case MONGOC_ERROR_QUERY:
    case MONGOC_ERROR_BSON:
      return Kind::kQuery;

    case MONGOC_ERROR_COMMAND:
      switch (value_.code) {
        case MONGOC_ERROR_COMMAND_INVALID_ARG:
          return Kind::kInvalidQueryArgument;

        case MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION:
          return Kind::kIncompatibleServer;

        default:;  // return kOther
      }
      break;

    case MONGOC_ERROR_COLLECTION:
      return Kind::kInvalidQueryArgument;

    case MONGOC_ERROR_SCRAM:
      return Kind::kAuthentication;

    case MONGOC_ERROR_SERVER_SELECTION:
      switch (value_.code) {
        case MONGOC_ERROR_SERVER_SELECTION_FAILURE:
          return Kind::kClusterUnavailable;

        default:;  // return kOther
      }
      break;

    case MONGOC_ERROR_WRITE_CONCERN:
      return Kind::kWriteConcern;

    case MONGOC_ERROR_SERVER:
      switch (value_.code) {
        case MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND:
          return Kind::kQuery;

        case MONGOC_ERROR_DUPLICATE_KEY:
          return Kind::kDuplicateKey;

        default:
          return Kind::kServer;
      }
      break;

    default:;  // return kOther
  }
  return Kind::kOther;
}

uint32_t MongoError::Code() const { return value_.code; }

const char* MongoError::Message() const { return value_.message; }

uint32_t MongoError::Domain() const { return value_.domain; }

bson_error_t* MongoError::GetNative() { return &value_; }

[[noreturn]] void MongoError::Throw(std::string prefix) const {
  if (!prefix.empty()) prefix += ": ";

  switch (GetKind()) {
    case Kind::kNoError:
      throw MongoException() << prefix << "error not set";

    case Kind::kNetwork:
      throw NetworkException() << prefix << value_.message;

    case Kind::kClusterUnavailable:
      throw ClusterUnavailableException() << prefix << value_.message;

    case Kind::kIncompatibleServer:
      throw IncompatibleServerException() << prefix << value_.message;

    case Kind::kAuthentication:
      throw AuthenticationException() << prefix << value_.message;

    case Kind::kQuery:
      throw QueryException() << prefix << value_.message;

    case Kind::kInvalidQueryArgument:
      throw InvalidQueryArgumentException() << prefix << value_.message;

    case Kind::kServer:
      throw ServerException(value_.code) << prefix << value_.message;

    case Kind::kWriteConcern:
      throw WriteConcernException(value_.code) << prefix << value_.message;

    case Kind::kDuplicateKey:
      throw DuplicateKeyException(value_.code) << prefix << value_.message;

    case Kind::kOther:
    default:;  // go to generic throw
  }
  throw MongoException() << prefix << '[' << value_.domain << ',' << value_.code
                         << "] " << value_.message;
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
