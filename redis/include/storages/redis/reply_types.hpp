#pragma once

#include <string>

#include <redis/base.hpp>
#include <redis/reply/ttl_reply.hpp>

namespace storages {
namespace redis {

enum class HsetReply { kCreated, kUpdated };

struct MemberScore final {
  std::string member;
  double score;
};

enum class SetReply { kSet, kNotSet };

enum class StatusOk { kOk };

enum class StatusPong { kPong };

using TtlReply = ::redis::TtlReply;

enum class TypeReply { kString, kList, kSet, kZset, kHash, kStream };

}  // namespace redis
}  // namespace storages
