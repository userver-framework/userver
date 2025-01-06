#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <librdkafka/rdkafka.h>

#include <userver/kafka/impl/stats.hpp>
#include <userver/utils/periodic_task.hpp>

#include <kafka/impl/concurrent_event_waiter.hpp>
#include <kafka/impl/delivery_waiter.hpp>
#include <kafka/impl/holders_aliases.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

class Configuration;

class ProducerImpl final {
public:
    explicit ProducerImpl(Configuration&& configuration);

    const Stats& GetStats() const;

    /// @brief Send the message and waits for its delivery.
    /// While waiting handles other messages delivery reports, errors and logs.
    [[nodiscard]] DeliveryResult Send(
        const std::string& topic_name,
        std::string_view key,
        std::string_view message,
        std::optional<std::uint32_t> partition
    ) const;

    /// @brief Waits until scheduled messages are delivered for
    /// at most 2 x `delivery_timeout`.
    ///
    /// @warning The function must be called before the destructor. After this
    /// call producer stops working and cannot be used anymore.
    void WaitUntilAllMessagesDelivered() &&;

    /// @brief Wakes a sleeping waiting to acknowledge about new events.
    void EventCallback();

private:
    /// @brief Shedules the message delivery.
    /// @returns the future for delivery result, which must be awaited.
    [[nodiscard]] engine::Future<DeliveryResult> ScheduleMessageDelivery(
        const std::string& topic_name,
        std::string_view key,
        std::string_view message,
        std::optional<std::uint32_t> partition
    ) const;

    /// @brief Poll a delivery or error event from producer's queue.
    EventHolder PollEvent() const;

    /// @brief Call a corresponding callback for the event data depends on its
    /// type.
    void DispatchEvent(const EventHolder& event_holder) const;

    /// @brief Polls and handles events from producer's queue, until queue is
    /// empty.
    /// @returns the number of handled events.
    std::size_t HandleEvents(std::string_view context) const;

    /// @brief Waits until message delivery status reported by `librdkafka`.
    /// Suspends for no more than `delivery_timeout` milliseconds.
    void WaitUntilDeliveryReported(engine::Future<DeliveryResult>& delivery_result) const;

    /// @brief Callback called on error in `librdkafka` work.
    void ErrorCallback(rd_kafka_resp_err_t error, const char* reason, bool is_fatal) const;

    /// @brief Callback called on debug `librdkafka` messages.
    void LogCallback(const char* facility, const char* message, int log_level) const;

    /// @brief Callback called on each succeeded/failed message delivery.
    /// @param message represents the delivered (or not) message. Its `_private`
    /// field contains and `opaque` argument, which was passed to
    /// `rd_kafka_producev`, i.e. the Promise which must be setted to notify
    /// waiter about the delivery.
    void DeliveryReportCallback(const rd_kafka_message_s* message) const;

private:
    const std::chrono::milliseconds delivery_timeout_;

    mutable Stats stats_;

    ConcurrentEventWaiters waiters_;
    ProducerHolder producer_;

    /// If no messages are send, some errors may occure and we want to log them
    /// anyway.
    utils::PeriodicTask log_events_handler_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
