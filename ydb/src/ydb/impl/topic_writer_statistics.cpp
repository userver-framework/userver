#include "topic_writer_statistics.hpp"

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

TopicWriterEventAcksStats& TopicWriterEventAcksStats::operator+=(const TopicWriterEventAcksStats& other) noexcept {
    written += other.written;
    already_written += other.already_written;
    discarded += other.discarded;
    written_in_tx += other.written_in_tx;
    unexpected += other.unexpected;
    last_acks += other.last_acks;
    return *this;
}

TopicWriterEventStats& TopicWriterEventStats::operator+=(const TopicWriterEventStats& other) noexcept {
    received += other.received;
    resource_exhasted += other.resource_exhasted;
    handled += other.handled;
    handled_failed += other.handled_failed;
    return *this;
}

TopicWriterMessagesStats& TopicWriterMessagesStats::operator+=(const TopicWriterMessagesStats& other) noexcept {
    received += other.received;
    resource_exhasted += other.resource_exhasted;
    handled += other.handled;
    handled_failed += other.handled_failed;
    return *this;
}

TopicWriterWorkerStats& TopicWriterWorkerStats::operator+=(const TopicWriterWorkerStats& other) noexcept {
    messages_stats += other.messages_stats;
    event_acks_stats += other.event_acks_stats;
    event_handled += other.event_handled;
    write_issues += other.write_issues;
    main_cycle_issue += other.main_cycle_issue;
    on_close_issue += other.on_close_issue;

    for (std::size_t i = 0; i < event_stats.size(); ++i) {
        event_stats[i] += other.event_stats[i];
    }

    return *this;
}

void DumpMetric(utils::statistics::Writer& writer, const TopicWriterEventStats& stats) {
    writer["received"] = stats.received;
    writer["resource_exhasted"] = stats.resource_exhasted;
    writer["handled"] = stats.handled;
    writer["handled_failed"] = stats.handled_failed;
}

void DumpMetric(utils::statistics::Writer& writer, const TopicWriterMessagesStats& stats) {
    writer["received"] = stats.received;
    writer["resource_exhasted"] = stats.resource_exhasted;
    writer["handled"] = stats.handled;
    writer["handled_failed"] = stats.handled_failed;
}

std::string_view TopicWriterEventTypeToString(TopicWriterEventType type) {
    switch (type) {
        case TopicWriterEventType::kAck:
            return "ack";
        case TopicWriterEventType::kReadyToAccept:
            return "ready_to_accept";
        case TopicWriterEventType::kSessionClosed:
            return "session_closed";
        case TopicWriterEventType::kMaxCount:
            UINVARIANT(false, "kMaxCount is not a valid event type");
            return "max_count";
    }
}

void DumpMetric(utils::statistics::Writer& writer, const TopicWriterWorkerStats& stats) {
    {
        auto event_writer = writer["ydb-events"];
        for (std::size_t i = 0; i < kEventStatsSize; ++i) {
            auto event_type = static_cast<TopicWriterEventType>(i);
            event_writer[std::string{TopicWriterEventTypeToString(event_type)}] = stats.event_stats[i];
        }
    }
    writer["messages"] = stats.messages_stats;
    writer["event_handled"] = stats.event_handled;

    writer["write_issues"] = stats.write_issues;
    writer["main_cycle_issues"] = stats.main_cycle_issue;
    writer["on_close_issue"] = stats.on_close_issue;

    writer["event_acks"] = stats.event_acks_stats;
}

void DumpMetric(utils::statistics::Writer& writer, const TopicWriterEventAcksStats& stats) {
    writer["written"] = stats.written;
    writer["already_written"] = stats.already_written;
    writer["discarded"] = stats.discarded;
    writer["written_in_tx"] = stats.written_in_tx;
    writer["unexpected"] = stats.unexpected;
    writer["last_acks"] = stats.last_acks;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
