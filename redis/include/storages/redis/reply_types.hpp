#pragma once

#include <string>
#include <vector>

#include <redis/base.hpp>
#include <redis/reply/expire_reply.hpp>
#include <redis/reply/ttl_reply.hpp>

#include <storages/redis/key_type.hpp>
#include <storages/redis/scan_tag.hpp>

namespace storages {
namespace redis {

using ReplyPtr = ::redis::ReplyPtr;

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

template <ScanTag scan_tag>
class ScanReplyTmpl final {
 public:
  using ReplyElem = typename ScanReplyElem<scan_tag>::type;

  static ScanReplyTmpl Parse(const ReplyPtr& reply,
                             const std::string& request_description);

  class Cursor final {
   public:
    Cursor() : Cursor(0) {}
    uint64_t GetValue() const { return value_; }

    friend class ScanReplyTmpl;

   private:
    explicit Cursor(uint64_t value) : value_(value) {}

    uint64_t value_;
  };

  const Cursor& GetCursor() const { return cursor_; }

  const std::vector<ReplyElem>& GetKeys() const { return keys_; }

  std::vector<ReplyElem>& GetKeys() { return keys_; }

 private:
  ScanReplyTmpl(Cursor cursor, std::vector<ReplyElem> keys)
      : cursor_(cursor), keys_(std::move(keys)) {}

  Cursor cursor_;
  std::vector<ReplyElem> keys_;
};

extern template class ScanReplyTmpl<ScanTag::kScan>;
extern template class ScanReplyTmpl<ScanTag::kSscan>;
extern template class ScanReplyTmpl<ScanTag::kHscan>;
extern template class ScanReplyTmpl<ScanTag::kZscan>;

using ScanReply = ScanReplyTmpl<ScanTag::kScan>;
using SscanReply = ScanReplyTmpl<ScanTag::kSscan>;
using HscanReply = ScanReplyTmpl<ScanTag::kHscan>;
using ZscanReply = ScanReplyTmpl<ScanTag::kZscan>;

enum class SetReply { kSet, kNotSet };

enum class StatusOk { kOk };

enum class StatusPong { kPong };

using TtlReply = ::redis::TtlReply;

}  // namespace redis
}  // namespace storages
