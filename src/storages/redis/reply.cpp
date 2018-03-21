#include "reply.hpp"

#include <cassert>
#include <sstream>

#include "base.hpp"

namespace storages {
namespace redis {

ReplyData::ReplyData(redisReply* reply) {
  if (!reply) return;

  switch (reply->type) {
    case REDIS_REPLY_STRING: {
      type_ = Type::kString;
      string_ = std::string(reply->str, reply->len);
      break;
    }
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

ReplyData::operator bool() const { return type_ != Type::kNoReply; }

ReplyData::Type ReplyData::GetType() const { return type_; }

const std::string& ReplyData::GetTypeString() const {
  static const std::string kTypeNoReply = "NO REPLY";
  static const std::string kTypeString = "string";
  static const std::string kTypeArray = "array";
  static const std::string kTypeInteger = "integer";
  static const std::string kTypeNil = "nil";
  static const std::string kTypeStatus = "status";
  static const std::string kTypeError = "error";
  static const std::string kTypeUnknown = "UNKNOWN";

  switch (type_) {
    case Type::kNoReply:
      return kTypeNoReply;
    case Type::kString:
      return kTypeString;
    case Type::kArray:
      return kTypeArray;
    case Type::kInteger:
      return kTypeInteger;
    case Type::kNil:
      return kTypeNil;
    case Type::kStatus:
      return kTypeStatus;
    case Type::kError:
      return kTypeError;
    default:
      return kTypeUnknown;
  }
}

bool ReplyData::IsString() const { return type_ == Type::kString; }

bool ReplyData::IsArray() const { return type_ == Type::kArray; }

bool ReplyData::IsInt() const { return type_ == Type::kInteger; }

bool ReplyData::IsNil() const { return type_ == Type::kNil; }

bool ReplyData::IsStatus() const { return type_ == Type::kStatus; }

bool ReplyData::IsError() const { return type_ == Type::kError; }

bool ReplyData::IsErrorMoved() const {
  return IsError() && !string_.compare(0, 6, "MOVED ");
}

bool ReplyData::IsErrorAsk() const {
  return IsError() && !string_.compare(0, 4, "ASK ");
}

const std::string& ReplyData::GetString() const {
  assert(IsString());
  return string_;
}

const ReplyData::ReplyArray& ReplyData::GetArray() const {
  assert(IsArray());
  return array_;
}

int64_t ReplyData::GetInt() const {
  assert(IsInt());
  return integer_;
}

const std::string& ReplyData::GetStatus() const {
  assert(IsStatus());
  return string_;
}

const std::string& ReplyData::GetError() const {
  assert(IsError());
  return string_;
}

const ReplyData& ReplyData::operator[](size_t idx) const {
  assert(IsArray());
  return array_.at(idx);
}

std::string ReplyData::ToString() const {
  switch (GetType()) {
    case ReplyData::Type::kNoReply:
      return std::string();
    case ReplyData::Type::kNil:
      return "(nil)";
    case ReplyData::Type::kString:
    case ReplyData::Type::kStatus:
    case ReplyData::Type::kError:
      return string_;
    case ReplyData::Type::kInteger:
      return std::to_string(integer_);
    case ReplyData::Type::kArray: {
      std::ostringstream os("");
      os << "[";
      for (size_t i = 0; i < array_.size(); i++) {
        if (i) os << ", ";
        os << array_[i].ToString();
      }
      os << "]";
      return os.str();
    }
    default:
      return "(unknown type)";
  }
}

const std::string& Reply::StatusString() const {
  static const std::string error = "error from hiredis";
  static const std::string ok = "OK";
  static const std::string other = "other error";
  static const std::string timeout = "timeout";
  static const std::string not_ready = "redis driver not ready";
  static const std::string unknown = "unknown status";

  switch (status) {
    case REDIS_ERR:
      return error;
    case REDIS_OK:
      return ok;
    case REDIS_ERR_OTHER:
      return other;
    case REDIS_ERR_TIMEOUT:
      return timeout;
    case REDIS_ERR_NOT_READY:
      return not_ready;
    default:
      return unknown;
  }
}

}  // namespace redis
}  // namespace storages
