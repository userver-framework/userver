#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

#include <boost/container/small_vector.hpp>

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

    void PushLap(std::string key, Duration value);

    /// Accumulated time for a certain key. If the key is not there, returns 0
    Duration DurationTotal(std::string_view key) const;

    void MergeInto(logging::impl::TagWriter writer);

private:
    struct Lap {
        std::string key;
        Duration duration;
    };

    static constexpr std::size_t kExpectedLapsCount = 6;

    boost::container::small_vector<Lap, kExpectedLapsCount> laps_;
};

}  // namespace tracing::impl

USERVER_NAMESPACE_END
