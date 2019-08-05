#pragma once

#include <string>
#include <vector>

#include <redis/base.hpp>
#include <redis/reply/expire_reply.hpp>
#include <redis/reply/ttl_reply.hpp>

#include <storages/redis/key_type.hpp>

namespace storages {
namespace redis {

using ExpireReply = ::redis::ExpireReply;

enum class HsetReply { kCreated, kUpdated };

struct MemberScore final {
  std::string member;
  double score;
};

enum class PersistReply { kKeyOrTimeoutNotFound, kTimeoutRemoved };

class ScanReply {
 public:
  static ScanReply Parse(::redis::ReplyPtr reply,
                         const std::string& request_description);

  class Cursor {
   public:
    Cursor() : Cursor(0) {}
    uint64_t GetValue() const { return value_; }

    friend class ScanReply;

   private:
    explicit Cursor(uint64_t value) : value_(value) {}

    uint64_t value_;
  };

  const Cursor& GetCursor() const { return cursor_; }

  const std::vector<std::string>& GetKeys() const { return keys_; }

  std::vector<std::string>& GetKeys() { return keys_; }

 private:
  ScanReply(Cursor cursor, std::vector<std::string> keys);

  Cursor cursor_;
  std::vector<std::string> keys_;
};

enum class SetReply { kSet, kNotSet };

enum class StatusOk { kOk };

enum class StatusPong { kPong };

using TtlReply = ::redis::TtlReply;

}  // namespace redis
}  // namespace storages
