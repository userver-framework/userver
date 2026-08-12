#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/logging/log_extra.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
class TagWriter;
}  // namespace logging::impl

namespace tracing::impl {

using PerfTimePoint = std::chrono::steady_clock::time_point;

class TimeStorage {
public:
    using Duration = std::chrono::nanoseconds;

    TimeStorage() = default;

    TimeStorage(const TimeStorage&) = delete;
    TimeStorage(TimeStorage&&) noexcept = default;

    void PushLap(std::string_view key, Duration value);

    /// Accumulated time for a certain key. If the key is not there, returns 0
    Duration DurationTotal(std::string_view key) const;

    void MergeInto(logging::impl::TagWriter writer);

private:
    utils::impl::TransparentMap<std::string, Duration> data_;
};

}  // namespace tracing::impl

USERVER_NAMESPACE_END
