#include <storages/mongo_ng/mongo_error.hpp>

#include <mongoc/mongoc.h>

#include <storages/mongo_ng/exception.hpp>

namespace storages::mongo_ng {

MongoError::MongoError() : value_{0, 0, '\0'} {}

MongoError::operator bool() const { return !!value_.code; }

// NOTE: must be manually synchronized with Throw
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

uint32_t MongoError::Code() const { return value_.code; }
const char* MongoError::Message() const { return value_.message; }
uint32_t MongoError::Domain() const { return value_.domain; }

bson_error_t* MongoError::GetNative() { return &value_; }

// NOTE: must be manually synchronized with IsServerError
[[noreturn]] void MongoError::Throw(std::string prefix) const {
  if (!prefix.empty()) prefix += ": ";

  switch (value_.domain) {
    case 0:
      throw MongoException(std::move(prefix)) << "unspecified error";

    case MONGOC_ERROR_CLIENT:
      switch (value_.code) {
        case MONGOC_ERROR_CLIENT_GETNONCE:
        case MONGOC_ERROR_CLIENT_AUTHENTICATE:
          throw AuthenticationException(std::move(prefix)) << value_.message;

        case MONGOC_ERROR_STREAM_CONNECT:
          throw NetworkException(std::move(prefix)) << value_.message;

        default:;  // go to generic throw
      }
      break;

    case MONGOC_ERROR_STREAM:
      switch (value_.code) {
        case MONGOC_ERROR_STREAM_NAME_RESOLUTION:
        case MONGOC_ERROR_STREAM_SOCKET:
        case MONGOC_ERROR_STREAM_CONNECT:
          throw NetworkException(std::move(prefix)) << value_.message;

        default:;  // go to generic throw
      }
      break;

    case MONGOC_ERROR_PROTOCOL:
      switch (value_.code) {
        case MONGOC_ERROR_PROTOCOL_INVALID_REPLY:
          throw NetworkException(std::move(prefix)) << value_.message;

        case MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION:
          throw IncompatibleServerException(std::move(prefix))
              << value_.message;

        default:;  // go to generic throw
      }
      break;

    case MONGOC_ERROR_CURSOR:
      switch (value_.code) {
        case MONGOC_ERROR_CURSOR_INVALID_CURSOR:
          throw QueryException(std::move(prefix)) << value_.message;

        default:;  // go to generic throw
      }
      break;

    case MONGOC_ERROR_QUERY:
    case MONGOC_ERROR_BSON:
      throw QueryException(std::move(prefix)) << value_.message;

    case MONGOC_ERROR_COMMAND:
      switch (value_.code) {
        case MONGOC_ERROR_COMMAND_INVALID_ARG:
          throw InvalidQueryArgumentException(std::move(prefix))
              << value_.message;

        case MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION:
          throw IncompatibleServerException(std::move(prefix))
              << value_.message;

        default:;  // go to generic throw
      }
      break;

    case MONGOC_ERROR_COLLECTION:
      throw InvalidQueryArgumentException(std::move(prefix)) << value_.message;

    case MONGOC_ERROR_SCRAM:
      throw AuthenticationException(std::move(prefix)) << value_.message;

    case MONGOC_ERROR_SERVER_SELECTION:
      switch (value_.code) {
        case MONGOC_ERROR_SERVER_SELECTION_FAILURE:
          throw ClusterUnavailableException(std::move(prefix))
              << value_.message;

        default:;  // go to generic throw
      }
      break;

    case MONGOC_ERROR_WRITE_CONCERN:
      throw WriteConcernException(value_.code, std::move(prefix))
          << value_.message;

    case MONGOC_ERROR_SERVER:
      switch (value_.code) {
        case MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND:
          throw QueryException(std::move(prefix)) << value_.message;

        case MONGOC_ERROR_DUPLICATE_KEY:
          throw DuplicateKeyException(value_.code, std::move(prefix))
              << value_.message;

        default:
          throw ServerException(value_.code, std::move(prefix))
              << value_.message;
      }
      break;

    default:;  // go to generic throw
  }
  throw MongoException(std::move(prefix))
      << '[' << value_.domain << ',' << value_.code << "] " << value_.message;
}

}  // namespace storages::mongo_ng
