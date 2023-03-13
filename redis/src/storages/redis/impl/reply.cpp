#include <userver/storages/redis/impl/reply.hpp>

#include <sstream>
#include <string>

#include <hiredis/hiredis.h>
#include <boost/algorithm/string/predicate.hpp>

#include <userver/storages/redis/impl/exception.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

ReplyData::ReplyData(const redisReply* reply) {
  if (!reply) return;

  switch (reply->type) {
    case REDIS_REPLY_STRING:
      type_ = Type::kString;
      string_ = std::string(reply->str, reply->len);
      break;
    case REDIS_REPLY_ARRAY:
      type_ = Type::kArray;
      array_.reserve(reply->elements);
      for (size_t i = 0; i < reply->elements; i++)
        array_.emplace_back(reply->element[i]);
      break;
    case REDIS_REPLY_INTEGER:
      type_ = Type::kInteger;
      integer_ = reply->integer;
      break;
    case REDIS_REPLY_NIL:
      type_ = Type::kNil;
      break;
    case REDIS_REPLY_STATUS:
      type_ = Type::kStatus;
      string_ = std::string(reply->str, reply->len);
      break;
    case REDIS_REPLY_ERROR:
      type_ = Type::kError;
      string_ = std::string(reply->str, reply->len);
      break;
    default:
      type_ = Type::kNoReply;
      break;
  }
}

ReplyData::ReplyData(Array&& array)
    : type_(Type::kArray), array_(std::move(array)) {}

ReplyData::ReplyData(std::string s)
    : type_(Type::kString), string_(std::move(s)) {}

ReplyData::ReplyData(int value) : type_(Type::kInteger), integer_(value) {}

ReplyData ReplyData::CreateError(std::string&& error_msg) {
  ReplyData data(std::move(error_msg));
  data.type_ = Type::kError;
  return data;
}

ReplyData ReplyData::CreateStatus(std::string&& status_msg) {
  ReplyData data(std::move(status_msg));
  data.type_ = Type::kStatus;
  return data;
}

ReplyData ReplyData::CreateNil() {
  ReplyData data;
  data.type_ = Type::kNil;
  return data;
}

std::string ReplyData::GetTypeString() const { return TypeToString(GetType()); }

std::string ReplyData::ToDebugString() const {
  switch (GetType()) {
    case ReplyData::Type::kNoReply:
      return {};
    case ReplyData::Type::kNil:
      return "(nil)";
    case ReplyData::Type::kString:
    case ReplyData::Type::kStatus:
    case ReplyData::Type::kError:
      return string_;
    case ReplyData::Type::kInteger:
      return std::to_string(integer_);
    case ReplyData::Type::kArray: {
      std::ostringstream os;
      os << "[";
      for (size_t i = 0; i < array_.size(); i++) {
        if (i) os << ", ";
        os << array_[i].ToDebugString();
      }
      os << "]";
      return os.str();
    }
    default:
      return "(unknown type)";
  }
}

ReplyData::KeyValues ReplyData::GetKeyValues() const {
  if (!IsArray())
    throw ParseReplyException(
        "Incorrect ReplyData type: expected kArray, found " +
        TypeToString(GetType()));
  if (GetArray().size() & 1)
    throw ParseReplyException("Array size is odd: " +
                              std::to_string(GetArray().size()));
  for (const auto& elem : GetArray())
    if (!elem.IsString())
      throw ParseReplyException("Non-string element (" + elem.GetTypeString() +
                                ')');
  return ReplyData::KeyValues(array_);
}

ReplyData::MovableKeyValues ReplyData::GetMovableKeyValues() {
  if (!IsArray())
    throw ParseReplyException(
        "Incorrect ReplyData type: expected kArray, found " +
        TypeToString(GetType()));
  if (GetArray().size() & 1)
    throw ParseReplyException("Array size is odd: " +
                              std::to_string(GetArray().size()));
  for (const auto& elem : GetArray())
    if (!elem.IsString())
      throw ParseReplyException("Non-string element (" + elem.GetTypeString() +
                                ')');
  return ReplyData::MovableKeyValues(array_);
}

std::string ReplyData::TypeToString(Type type) {
  switch (type) {
    case Type::kNoReply:
      return "kNoReply";
    case Type::kString:
      return "kString";
    case Type::kArray:
      return "kArray";
    case Type::kInteger:
      return "kInteger";
    case Type::kNil:
      return "kNil";
    case Type::kStatus:
      return "kStatus";
    case Type::kError:
      return "kError";
  };

  return "Unknown (" + std::to_string(static_cast<int>(type)) + ")";
}

size_t ReplyData::GetSize() const {
  size_t sum = 0;

  switch (type_) {
    case Type::kNoReply:
      return 1;

    case Type::kArray:
      for (const auto& data : array_) sum += data.GetSize();
      return sum;

    case Type::kInteger:
      return sizeof(integer_);

    case Type::kNil:
      return 1;

    case Type::kString:
    case Type::kStatus:
    case Type::kError:
      return string_.size();
  }
  return 1;
}

bool ReplyData::IsUnusableInstanceError() const {
  if (IsError()) {
    const auto& msg = GetError();

    if (boost::starts_with(msg.c_str(), "MASTERDOWN ")) return true;
    if (boost::starts_with(msg.c_str(), "LOADING ")) return true;
  }

  return false;
}

bool ReplyData::IsReadonlyError() const {
  if (IsError()) {
    const auto& msg = GetError();

    if (boost::starts_with(msg.c_str(), "READONLY ")) return true;
  }

  return false;
}

bool ReplyData::IsUnknownCommandError() const {
  if (IsError()) {
    const auto& msg = GetError();

    if (boost::starts_with(msg.c_str(), "ERR unknown command ")) return true;
  }

  return false;
}

void ReplyData::ExpectType(ReplyData::Type type,
                           const std::string& request_description) const {
  if (GetType() != type) ThrowUnexpectedReplyType(type, request_description);
}

void ReplyData::ExpectString(const std::string& request_description) const {
  ExpectType(ReplyData::Type::kString, request_description);
}

void ReplyData::ExpectArray(const std::string& request_description) const {
  ExpectType(ReplyData::Type::kArray, request_description);
}

void ReplyData::ExpectInt(const std::string& request_description) const {
  ExpectType(ReplyData::Type::kInteger, request_description);
}

void ReplyData::ExpectNil(const std::string& request_description) const {
  ExpectType(ReplyData::Type::kNil, request_description);
}

void ReplyData::ExpectStatus(const std::string& request_description) const {
  ExpectType(ReplyData::Type::kStatus, request_description);
}

void ReplyData::ExpectStatusEqualTo(
    const std::string& expected_status_str,
    const std::string& request_description) const {
  ExpectStatus(request_description);
  if (GetStatus() != expected_status_str) {
    throw ParseReplyException(
        "Unexpected redis reply to '" + request_description +
        "' request: expected status=" + expected_status_str +
        ", got status=" + GetStatus());
  }
}

void ReplyData::ExpectError(const std::string& request_description) const {
  ExpectType(ReplyData::Type::kError, request_description);
}

[[noreturn]] void ReplyData::ThrowUnexpectedReplyType(
    ReplyData::Type expected, const std::string& request_description) const {
  throw ParseReplyException(
      "Unexpected redis reply type to '" + request_description +
      "' request: expected " + ReplyData::TypeToString(expected) +
      ", got type=" + GetTypeString() + " data=" + ToDebugString());
}

Reply::Reply(std::string cmd, redisReply* redis_reply, ReplyStatus status)
    : cmd(std::move(cmd)), data(redis_reply), status(status) {}

Reply::Reply(std::string cmd, redisReply* redis_reply, ReplyStatus status,
             std::string status_string)
    : cmd(std::move(cmd)),
      data(redis_reply),
      status(status),
      status_string(std::move(status_string)) {}

Reply::Reply(std::string cmd, ReplyData&& data)
    : cmd(std::move(cmd)), data(std::move(data)), status(ReplyStatus::kOk) {}

bool Reply::IsOk() const { return status == ReplyStatus::kOk; }

bool Reply::IsLoggableError() const {
  /* In case of not loggable error just filter it out */
  return !IsOk();
}

bool Reply::IsUnusableInstanceError() const {
  return data.IsUnusableInstanceError();
}

bool Reply::IsReadonlyError() const { return data.IsReadonlyError(); }

bool Reply::IsUnknownCommandError() const {
  return data.IsUnknownCommandError();
}

const logging::LogExtra& Reply::GetLogExtra() const { return log_extra; }

void Reply::FillSpanTags(tracing::Span& span) const {
  span.AddTag(tracing::kDatabaseType, tracing::kDatabaseRedisType);
  span.AddTag(tracing::kPeerAddress, server);
}

void Reply::ExpectIsOk(const std::string& request_description) const {
  if (!IsOk()) {
    throw RequestFailedException(GetRequestDescription(request_description),
                                 status);
  }
}

void Reply::ExpectType(ReplyData::Type type,
                       const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectType(type, GetRequestDescription(request_description));
}

void Reply::ExpectString(const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectString(GetRequestDescription(request_description));
}

void Reply::ExpectArray(const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectArray(GetRequestDescription(request_description));
}

void Reply::ExpectInt(const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectInt(GetRequestDescription(request_description));
}

void Reply::ExpectNil(const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectNil(GetRequestDescription(request_description));
}

void Reply::ExpectStatus(const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectStatus(GetRequestDescription(request_description));
}

void Reply::ExpectStatusEqualTo(const std::string& expected_status_str,
                                const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectStatusEqualTo(expected_status_str,
                           GetRequestDescription(request_description));
}

void Reply::ExpectError(const std::string& request_description) const {
  ExpectIsOk(request_description);
  data.ExpectError(GetRequestDescription(request_description));
}

const std::string& Reply::GetRequestDescription(
    const std::string& request_description) const {
  return request_description.empty() ? cmd : request_description;
}

}  // namespace redis

USERVER_NAMESPACE_END
