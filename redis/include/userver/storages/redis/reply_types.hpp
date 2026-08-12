#pragma once

/// @file
/// @brief Definitions of structures representing different Redis replies.

#include <string>
#include <vector>

#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/expire_reply.hpp>
#include <userver/storages/redis/ttl_reply.hpp>

#include <userver/storages/redis/key_type.hpp>
#include <userver/storages/redis/reply_fwd.hpp>
#include <userver/storages/redis/scan_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

enum class HsetReply { kCreated, kUpdated };

/// @brief Result of HSETEX.
enum class HsetexReply : std::int8_t {
    kConditionNotMet = 0,  ///< FNX/FXX condition failed; no fields written
    kFieldsSet = 1,        ///< Fields written
};

struct Point {
    double lon;
    double lat;

    bool operator==(const Point& rhs) const { return std::tie(lon, lat) == std::tie(rhs.lon, rhs.lat); }
};

struct GeoPoint final {
    std::string member;
    std::optional<double> dist;
    std::optional<uint64_t> hash;
    std::optional<Point> point;

    GeoPoint() = default;

    GeoPoint(std::string member, std::optional<double> dist, std::optional<uint64_t> hash, std::optional<Point> point)
        : member(std::move(member)),
          dist(dist),
          hash(hash),
          point(point)
    {}

    bool operator==(const GeoPoint& rhs) const {
        return std::tie(member, dist, hash, point) == std::tie(rhs.member, rhs.dist, rhs.hash, rhs.point);
    }
};

/// @brief Data type that holds `member` and `score`.
///
/// Sample usage:
/// @snippet redis/src/storages/redis/client_scan_redistest.cpp  Sample Zscan usage
struct MemberScore final {
    std::string member;
    double score{0.0};

    MemberScore() = default;
    MemberScore(std::string member, double score)
        : member(std::move(member)),
          score(score)
    {}

    operator std::pair<std::string, double>() const& { return {member, score}; }

    operator std::pair<std::string, double>() && { return {std::move(member), score}; }

    operator std::pair<const std::string, double>() const& { return {member, score}; }

    operator std::pair<const std::string, double>() && { return {std::move(member), score}; }

    bool operator==(const MemberScore& rhs) const { return member == rhs.member && score == rhs.score; }
};

enum class PersistReply { kKeyOrTimeoutNotFound, kTimeoutRemoved };

/// @brief Per-field result of HEXPIRE / HPEXPIRE / HEXPIREAT / HPEXPIREAT.
enum class HexpireReply : std::int8_t {
    kFieldDoesNotExist = -2,
    kConditionNotMet = 0,    ///< NX/XX/GT/LT predicate failed
    kExpirationUpdated = 1,  ///< TTL applied
    kFieldDeleted = 2,       ///< ttl <= 0 / already in the past — field removed
};

/// @brief Per-field result of HPERSIST.
enum class HpersistReply : std::int8_t {
    kFieldDoesNotExist = -2,
    kFieldHasNoExpiration = -1,
    kExpirationRemoved = 1,
};

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

}  // namespace storages::redis

USERVER_NAMESPACE_END
