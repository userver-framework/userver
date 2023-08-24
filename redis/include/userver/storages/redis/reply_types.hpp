#pragma once

#include <string>
#include <vector>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/reply/expire_reply.hpp>
#include <userver/storages/redis/impl/reply/ttl_reply.hpp>
#include <userver/utils/void_t.hpp>

#include <userver/storages/redis/key_type.hpp>
#include <userver/storages/redis/reply_fwd.hpp>
#include <userver/storages/redis/scan_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using ExpireReply = USERVER_NAMESPACE::redis::ExpireReply;

enum class HsetReply { kCreated, kUpdated };

struct Point {
  double lon;
  double lat;

  bool operator==(const Point& rhs) const {
    return std::tie(lon, lat) == std::tie(rhs.lon, rhs.lat);
  }
};

struct GeoPoint final {
  std::string member;
  std::optional<double> dist;
  std::optional<uint64_t> hash;
  std::optional<Point> point;

  GeoPoint() = default;

  GeoPoint(std::string member, std::optional<double> dist,
           std::optional<uint64_t> hash, std::optional<Point> point)
      : member(std::move(member)), dist(dist), hash(hash), point(point) {}

  bool operator==(const GeoPoint& rhs) const {
    return std::tie(member, dist, hash, point) ==
           std::tie(rhs.member, rhs.dist, rhs.hash, rhs.point);
  }

  bool operator!=(const GeoPoint& rhs) const { return !(*this == rhs); }
};

struct MemberScore final {
  std::string member;
  double score{0.0};

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

  bool operator!=(const MemberScore& rhs) const { return !(*this == rhs); }
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

using TtlReply = USERVER_NAMESPACE::redis::TtlReply;

}  // namespace storages::redis

USERVER_NAMESPACE_END
