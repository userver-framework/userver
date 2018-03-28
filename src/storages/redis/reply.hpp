#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <hiredis/hiredis.h>

namespace storages {
namespace redis {

class ReplyData {
 public:
  typedef std::vector<ReplyData> ReplyArray;

  enum class Type {
    kNoReply,
    kString,
    kArray,
    kInteger,
    kNil,
    kStatus,
    kError,
  };

  ReplyData(redisReply* reply);
  ~ReplyData() {}

  explicit operator bool() const;

  Type GetType() const;
  const std::string& GetTypeString() const;

  bool IsString() const;
  bool IsArray() const;
  bool IsInt() const;
  bool IsNil() const;
  bool IsStatus() const;
  bool IsError() const;

  bool IsErrorMoved() const;
  bool IsErrorAsk() const;

  const std::string& GetString() const;
  const ReplyArray& GetArray() const;
  int64_t GetInt() const;
  const std::string& GetStatus() const;
  const std::string& GetError() const;
  const ReplyData& operator[](size_t idx) const;
  std::string ToString() const;

 private:
  Type type_ = Type::kNoReply;

  int64_t integer_;
  ReplyArray array_;
  std::string string_;
};

class Reply {
 public:
  Reply(const std::string& _cmd, redisReply* reply, int _status)
      : cmd(_cmd), data(reply), status(_status) {}
  ~Reply() {}

  std::string cmd;
  ReplyData data;
  int status;
  std::chrono::duration<double> time{0};

  explicit operator bool() const { return IsOk(); }
  bool IsOk() const { return status == REDIS_OK; }
  const std::string& StatusString() const;
};

using ReplyPtr = std::shared_ptr<Reply>;

}  // namespace redis
}  // namespace storages
