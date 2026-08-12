#pragma once

#include <algorithm>
#include <array>

#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/utils/statistics/relaxed_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

using RateCounter = utils::statistics::RateCounter;

enum class TopicWriterEventType { kAck, kReadyToAccept, kSessionClosed, kMaxCount };

std::string_view TopicWriterEventTypeToString(TopicWriterEventType type);

using GaugeCounter = utils::statistics::RelaxedCounter<std::size_t>;

inline constexpr std::size_t kEventStatsSize = static_cast<std::size_t>(TopicWriterEventType::kMaxCount);

struct TopicWriterEventAcksStats {
    RateCounter written{0};
    RateCounter already_written{0};
    RateCounter discarded{0};
    RateCounter written_in_tx{0};
    RateCounter unexpected{0};
    GaugeCounter last_acks{0};

    TopicWriterEventAcksStats& operator+=(const TopicWriterEventAcksStats& other) noexcept;

    friend void DumpMetric(utils::statistics::Writer& writer, const TopicWriterEventAcksStats& stats);
};

struct TopicWriterEventStats {
    RateCounter received{0};           // received by handler
    RateCounter resource_exhasted{0};  // was not handled in time in event handler
    RateCounter handled{0};            // handled by main loop
    RateCounter handled_failed{0};

    TopicWriterEventStats& operator+=(const TopicWriterEventStats& other) noexcept;

    friend void DumpMetric(utils::statistics::Writer& writer, const TopicWriterEventStats& stats);
};

struct TopicWriterMessagesStats {
    RateCounter received{0};           // received by handler
    RateCounter resource_exhasted{0};  // was not handled in time in event handler
    RateCounter handled{0};            // handled by main loop
    RateCounter handled_failed{0};

    TopicWriterMessagesStats& operator+=(const TopicWriterMessagesStats& other) noexcept;

    friend void DumpMetric(utils::statistics::Writer& writer, const TopicWriterMessagesStats& stats);
};

struct TopicWriterWorkerStats {
    TopicWriterMessagesStats messages_stats;
    TopicWriterEventAcksStats event_acks_stats;
    RateCounter event_handled{0};
    std::array<TopicWriterEventStats, kEventStatsSize> event_stats{};
    RateCounter write_issues{0};
    RateCounter main_cycle_issue{0};
    RateCounter on_close_issue{0};

    TopicWriterEventStats& GetAckEventStats() {
        static_assert(static_cast<std::size_t>(TopicWriterEventType::kAck) < kEventStatsSize);
        return event_stats[static_cast<std::size_t>(TopicWriterEventType::kAck)];
    }

    TopicWriterEventStats& GetReadyToAcceptEventStats() {
        static_assert(static_cast<std::size_t>(TopicWriterEventType::kReadyToAccept) < kEventStatsSize);
        return event_stats[static_cast<std::size_t>(TopicWriterEventType::kReadyToAccept)];
    }

    TopicWriterEventStats& GetSessionClosedEventStats() {
        static_assert(static_cast<std::size_t>(TopicWriterEventType::kSessionClosed) < kEventStatsSize);
        return event_stats[static_cast<std::size_t>(TopicWriterEventType::kSessionClosed)];
    }

    TopicWriterWorkerStats& operator+=(const TopicWriterWorkerStats& other) noexcept;

    friend void DumpMetric(utils::statistics::Writer& writer, const TopicWriterWorkerStats& stats);
};

}  // namespace ydb::impl

USERVER_NAMESPACE_END
