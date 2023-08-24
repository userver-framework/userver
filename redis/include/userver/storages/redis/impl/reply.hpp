#pragma once

#include <cassert>
#include <string>
#include <vector>

#include <userver/logging/log_extra.hpp>
#include <userver/utils/assert.hpp>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/reply_status.hpp>

struct redisReply;

USERVER_NAMESPACE_BEGIN

namespace redis {

class ReplyData final {
 public:
  using Array = std::vector<ReplyData>;

  enum class Type {
    kNoReply,
    kString,
    kArray,
    kInteger,
    kNil,
    kStatus,
    kError,
  };

  class KeyValues final {
   public:
    class KeyValue final {
     public:
      KeyValue(const Array& array, size_t index)
          : array_(array), index_(index) {}

      std::string Key() const { return array_[index_ * 2].GetString(); }
      std::string Value() const { return array_[index_ * 2 + 1].GetString(); }

     private:
      const Array& array_;
      size_t index_;
    };

    class KeyValueIt final {
     public:
      KeyValueIt(const Array& array, size_t index)
          : array_(array), index_(index) {}
      KeyValueIt& operator++() {
        ++index_;
        return *this;
      }
      bool operator!=(const KeyValueIt& r) const { return index_ != r.index_; }
      KeyValue operator*() const { return {array_, index_}; }

     private:
      const Array& array_;
      size_t index_;
    };

    explicit KeyValues(const Array& array) : array_(array) {}

    KeyValueIt begin() const { return {array_, 0}; }
    KeyValueIt end() const { return {array_, size()}; }

    size_t size() const { return array_.size() / 2; }

   private:
    const Array& array_;
  };

  class MovableKeyValues final {
   public:
    class MovableKeyValue final {
     public:
      MovableKeyValue(ReplyData& key_data, ReplyData& value_data)
          : key_data_(key_data), value_data_(value_data) {}

      std::string& Key() { return key_data_.GetString(); }
      std::string& Value() { return value_data_.GetString(); }

     private:
      ReplyData& key_data_;
      ReplyData& value_data_;
    };

    class MovableKeyValueIt final {
     public:
      MovableKeyValueIt(Array& array, size_t index)
          : array_(array), index_(index) {}
      MovableKeyValueIt& operator++() {
        ++index_;
        return *this;
      }
      bool operator!=(const MovableKeyValueIt& r) const {
        return index_ != r.index_;
      }
      MovableKeyValue operator*() {
        return {array_[index_ * 2], array_[index_ * 2 + 1]};
      }

     private:
      Array& array_;
      size_t index_;
    };

    explicit MovableKeyValues(Array& array) : array_(array) {}

    MovableKeyValueIt begin() const { return {array_, 0}; }
    MovableKeyValueIt end() const { return {array_, size()}; }

    size_t size() const { return array_.size() / 2; }

   private:
    Array& array_;
  };

  MovableKeyValues GetMovableKeyValues();

  ReplyData(const redisReply* reply);
  ReplyData(Array&& array);
  ReplyData(std::string s);
  ReplyData(int value);
  static ReplyData CreateError(std::string&& error_msg);
  static ReplyData CreateStatus(std::string&& status_msg);
  static ReplyData CreateNil();

  explicit operator bool() const { return type_ != Type::kNoReply; }

  Type GetType() const { return type_; }
  std::string GetTypeString() const;

  inline bool IsString() const { return type_ == Type::kString; }
  inline bool IsArray() const { return type_ == Type::kArray; }
  inline bool IsInt() const { return type_ == Type::kInteger; }
  inline bool IsNil() const { return type_ == Type::kNil; }
  inline bool IsStatus() const { return type_ == Type::kStatus; }
  inline bool IsError() const { return type_ == Type::kError; }
  bool IsUnusableInstanceError() const;
  bool IsReadonlyError() const;
  bool IsUnknownCommandError() const;

  bool IsErrorMoved() const {
    return IsError() && !string_.compare(0, 6, "MOVED ");
  }

  bool IsErrorAsk() const {
    return IsError() && !string_.compare(0, 4, "ASK ");
  }

  const std::string& GetString() const {
    UASSERT(IsString());
    return string_;
  }

  std::string& GetString() {
    UASSERT(IsString());
    return string_;
  }

  const Array& GetArray() const {
    UASSERT(IsArray());
    return array_;
  }

  Array& GetArray() {
    UASSERT(IsArray());
    return array_;
  }

  int64_t GetInt() const {
    UASSERT(IsInt());
    return integer_;
  }

  const std::string& GetStatus() const {
    UASSERT(IsStatus());
    return string_;
  }

  std::string& GetStatus() {
    UASSERT(IsStatus());
    return string_;
  }

  const std::string& GetError() const {
    UASSERT(IsError());
    return string_;
  }

  std::string& GetError() {
    UASSERT(IsError());
    return string_;
  }

  const ReplyData& operator[](size_t idx) const {
    UASSERT(IsArray());
    return array_.at(idx);
  }

  ReplyData& operator[](size_t idx) {
    UASSERT(IsArray());
    return array_.at(idx);
  }

  size_t GetSize() const;

  std::string ToDebugString() const;
  KeyValues GetKeyValues() const;
  static std::string TypeToString(Type type);

  void ExpectType(ReplyData::Type type,
                  const std::string& request_description = {}) const;

  void ExpectString(const std::string& request_description = {}) const;
  void ExpectArray(const std::string& request_description = {}) const;
  void ExpectInt(const std::string& request_description = {}) const;
  void ExpectNil(const std::string& request_description = {}) const;
  void ExpectStatus(const std::string& request_description = {}) const;
  void ExpectStatusEqualTo(const std::string& expected_status_str,
                           const std::string& request_description = {}) const;
  void ExpectError(const std::string& request_description = {}) const;

 private:
  ReplyData() = default;

  [[noreturn]] void ThrowUnexpectedReplyType(
      ReplyData::Type expected, const std::string& request_description) const;

  Type type_ = Type::kNoReply;

  int64_t integer_{};
  Array array_;
  std::string string_;
};

class Reply final {
 public:
  Reply(std::string cmd, redisReply* redis_reply, ReplyStatus status);
  Reply(std::string cmd, redisReply* redis_reply, ReplyStatus status,
        std::string status_string);
  Reply(std::string cmd, ReplyData&& data);

  std::string server;
  ServerId server_id;
  std::string cmd;
  ReplyData data;
  ReplyStatus status;
  std::string status_string;
  double time = 0.0;
  logging::LogExtra log_extra;

  operator bool() const { return IsOk(); }

  bool IsOk() const;
  bool IsLoggableError() const;
  bool IsUnusableInstanceError() const;
  bool IsReadonlyError() const;
  bool IsUnknownCommandError() const;
  const logging::LogExtra& GetLogExtra() const;
  void FillSpanTags(tracing::Span& span) const;

  void ExpectIsOk(const std::string& request_description = {}) const;
  void ExpectType(ReplyData::Type type,
                  const std::string& request_description = {}) const;

  void ExpectString(const std::string& request_description = {}) const;
  void ExpectArray(const std::string& request_description = {}) const;
  void ExpectInt(const std::string& request_description = {}) const;
  void ExpectNil(const std::string& request_description = {}) const;
  void ExpectStatus(const std::string& request_description = {}) const;
  void ExpectStatusEqualTo(const std::string& expected_status_str,
                           const std::string& request_description = {}) const;
  void ExpectError(const std::string& request_description = {}) const;

  const std::string& GetRequestDescription(
      const std::string& request_description) const;
};

}  // namespace redis

USERVER_NAMESPACE_END
