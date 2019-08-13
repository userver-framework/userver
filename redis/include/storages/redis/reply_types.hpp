#pragma once

#include <string>
#include <vector>

#include <redis/base.hpp>
#include <redis/reply/expire_reply.hpp>
#include <redis/reply/ttl_reply.hpp>
#include <utils/void_t.hpp>

#include <storages/redis/key_type.hpp>
#include <storages/redis/scan_tag.hpp>

namespace storages {
namespace redis {

using ReplyPtr = ::redis::ReplyPtr;

namespace impl {

template <typename Result, typename = ::utils::void_t<>>
struct DefaultReplyTypeHelper {
  using type = Result;
};

template <typename Result>
struct DefaultReplyTypeHelper<Result,
                              ::utils::void_t<decltype(Result::Parse(
                                  std::declval<const ReplyPtr&>(),
                                  std::declval<const std::string&>()))>> {
  using type = decltype(Result::Parse(std::declval<const ReplyPtr&>(),
                                      std::declval<const std::string&>()));
};

template <typename Result>
using DefaultReplyType = typename DefaultReplyTypeHelper<Result>::type;

}  // namespace impl

using ExpireReply = ::redis::ExpireReply;

enum class HsetReply { kCreated, kUpdated };

struct MemberScore final {
  std::string member;
  double score;

  MemberScore() = default;
  MemberScore(std::string member, double score)
      : member(std::move(member)), score(score) {}

  operator std::pair<std::string, double>() const& { return {member, score}; }

  operator std::pair<std::string, double>() && {
    return {std::move(member), score};
  }

  operator std::pair<const std::string, double>() const& {
    return {member, score};
  }

  operator std::pair<const std::string, double>() && {
    return {std::move(member), score};
  }

  bool operator==(const MemberScore& rhs) const {
    return member == rhs.member && score == rhs.score;
  }
};

enum class PersistReply { kKeyOrTimeoutNotFound, kTimeoutRemoved };

template <ScanTag>
struct ScanReplyElem;

template <>
struct ScanReplyElem<ScanTag::kScan> {
  using type = std::string;
};

template <>
struct ScanReplyElem<ScanTag::kSscan> {
  using type = std::string;
};

template <>
struct ScanReplyElem<ScanTag::kHscan> {
  using type = std::pair<std::string, std::string>;
};

template <>
struct ScanReplyElem<ScanTag::kZscan> {
  using type = MemberScore;
};

enum class SetReply { kSet, kNotSet };

enum class StatusOk { kOk };

enum class StatusPong { kPong };

using TtlReply = ::redis::TtlReply;

}  // namespace redis
}  // namespace storages
