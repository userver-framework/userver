#pragma once

/// @file
/// @brief Definitions of structures representing options for different commands

#include <optional>
#include <string>
#include <vector>

#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/scan_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using Longitude = utils::StrongTypedef<struct LongitudeTag, double>;
using Latitude = utils::StrongTypedef<struct LatitudeTag, double>;
using BoxWidth = utils::StrongTypedef<struct BoxWidthTag, double>;
using BoxHeight = utils::StrongTypedef<struct BoxHeightTag, double>;

struct RangeOptions {
    std::optional<size_t> offset;
    std::optional<size_t> count;
};

struct GeoaddArg {
    double lon;
    double lat;
    std::string member;
};

struct GeoradiusOptions {
    enum class Sort { kNone, kAsc, kDesc };
    enum class Unit { kM, kKm, kMi, kFt };

    Unit unit = Unit::kM;
    bool withcoord = false;
    bool withdist = false;
    bool withhash = false;
    size_t count = 0;
    Sort sort = Sort::kNone;
};

struct GeosearchOptions {
    enum class Sort { kNone, kAsc, kDesc };
    enum class Unit { kM, kKm, kMi, kFt };

    Unit unit = Unit::kM;
    bool withcoord = false;
    bool withdist = false;
    bool withhash = false;
    size_t count = 0;
    Sort sort = Sort::kNone;
};

struct ZaddOptions {
    enum class Exist { kAddAlways, kAddIfNotExist, kAddIfExist };
    enum class Compare { kNone, kGreaterThan, kLessThan };
    enum class ReturnValue { kAddedCount, kChangedCount };

    ZaddOptions() = default;
    constexpr ZaddOptions(
        Exist exist,
        ReturnValue return_value = ReturnValue::kAddedCount,
        Compare compare = Compare::kNone
    )
        : exist(exist), compare(compare), return_value(return_value) {}
    constexpr ZaddOptions(Exist exist, Compare compare, ReturnValue return_value = ReturnValue::kAddedCount)
        : exist(exist), compare(compare), return_value(return_value) {}

    constexpr ZaddOptions(ReturnValue return_value, Exist exist = Exist::kAddAlways, Compare compare = Compare::kNone)
        : exist(exist), compare(compare), return_value(return_value) {}
    constexpr ZaddOptions(ReturnValue return_value, Compare compare, Exist exist = Exist::kAddAlways)
        : exist(exist), compare(compare), return_value(return_value) {}

    constexpr ZaddOptions(
        Compare compare,
        Exist exist = Exist::kAddAlways,
        ReturnValue return_value = ReturnValue::kAddedCount
    )
        : exist(exist), compare(compare), return_value(return_value) {}
    constexpr ZaddOptions(Compare compare, ReturnValue return_value, Exist exist = Exist::kAddAlways)
        : exist(exist), compare(compare), return_value(return_value) {}

    Exist exist = Exist::kAddAlways;
    Compare compare = Compare::kNone;
    ReturnValue return_value = ReturnValue::kAddedCount;
};

constexpr ZaddOptions operator|(ZaddOptions::Exist exist, ZaddOptions::ReturnValue return_value) {
    return {exist, return_value};
}
constexpr ZaddOptions operator|(ZaddOptions::Exist exist, ZaddOptions::Compare compare) { return {exist, compare}; }
constexpr ZaddOptions operator|(ZaddOptions::Compare compare, ZaddOptions::Exist exist) { return {compare, exist}; }
constexpr ZaddOptions operator|(ZaddOptions::Compare compare, ZaddOptions::ReturnValue return_value) {
    return {compare, return_value};
}
constexpr ZaddOptions operator|(ZaddOptions::ReturnValue return_value, ZaddOptions::Exist exist) {
    return {return_value, exist};
}
constexpr ZaddOptions operator|(ZaddOptions::ReturnValue return_value, ZaddOptions::Compare compare) {
    return {return_value, compare};
}

/// @brief A command option to use in Scan, Hscan, Scan and Zscan commands.
///
/// Sample usage:
/// @snippet redis/src/storages/redis/client_scan_redistest.cpp  Sample Scan usage
class Match final {
public:
    explicit Match(std::string value) : value_(std::move(value)) {}

    const std::string& Get() const& { return value_; }

    std::string Get() && { return std::move(value_); }

private:
    std::string value_;
};

/// @brief A hint for Scan, Hscan, Scan and Zscan commands.
class Count final {
public:
    explicit constexpr Count(std::size_t value) noexcept : value_(value) {}

    constexpr std::size_t Get() const noexcept { return value_; }

private:
    std::size_t value_;
};

/// @brief A command option to use in Scan, Hscan, Scan and Zscan commands that combines Match and Count options.
class ScanOptionsGeneric {
public:
    using Match = storages::redis::Match;
    using Count = storages::redis::Count;

    ScanOptionsGeneric() = default;

    ScanOptionsGeneric(const ScanOptionsGeneric& other) = default;
    ScanOptionsGeneric& operator=(const ScanOptionsGeneric& other) = default;

    ScanOptionsGeneric(ScanOptionsGeneric&& other) = default;
    ScanOptionsGeneric& operator=(ScanOptionsGeneric&& other) = default;

    template <typename... Args>
    ScanOptionsGeneric(Args... args) {
        (Apply(std::move(args)), ...);
    }

    const std::optional<Match>& GetMatchOptional() const noexcept { return pattern_; }

    const std::optional<Count>& GetCountOptional() const noexcept { return count_; }

    std::optional<Match> ExtractMatch() noexcept { return std::move(pattern_); }

    std::optional<Count> ExtractCount() noexcept { return std::move(count_); }

private:
    void Apply(Match pattern) {
        if (pattern_) throw InvalidArgumentException("duplicate Match parameter");
        pattern_ = std::move(pattern);
    }

    void Apply(Count count) {
        if (count_) throw InvalidArgumentException("duplicate Count parameter");
        count_ = count;
    }

    std::optional<Match> pattern_;
    std::optional<Count> count_;
};

template <ScanTag scan_tag>
using ScanOptionsTmpl [[deprecated("Just use ScanOptionsGeneric")]] = ScanOptionsGeneric;

using ScanOptions = ScanOptionsGeneric;
using SscanOptions = ScanOptionsGeneric;
using HscanOptions = ScanOptionsGeneric;
using ZscanOptions = ScanOptionsGeneric;

struct SetOptions {
    enum class Exist { kSetAlways, kSetIfNotExist, kSetIfExist };

    int seconds = 0;
    int milliseconds = 0;
    Exist exist = Exist::kSetAlways;
};

struct ScoreOptions {
    bool withscores = false;
};

struct RangeScoreOptions {
    ScoreOptions score_options;
    RangeOptions range_options;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
