#include <kafka/impl/producer_impl.hpp>

#include <string>
#include <utility>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/trivial_map.hpp>

#include <kafka/impl/configuration.hpp>
#include <kafka/impl/log_level.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

std::chrono::milliseconds GetMessageLatency(const rd_kafka_message_t* message) {
  const std::chrono::microseconds message_latency_micro{
      rd_kafka_message_latency(message)};

  return std::chrono::duration_cast<std::chrono::milliseconds>(
      message_latency_micro);
}

void EventCallbackProxy(rd_kafka_t* producer, void* opaque_ptr) {
  UASSERT(producer);
  UASSERT(opaque_ptr);

  static_cast<ProducerImpl*>(opaque_ptr)->EventCallback();
}

}  // namespace

void ProducerImpl::ErrorCallback(rd_kafka_resp_err_t error, const char* reason,
                                 bool is_fatal) const {
  tracing::Span span{"error_callback"};
  span.AddTag("kafka_callback", "error_callback");

  LOG(is_fatal ? logging::Level::kCritical : logging::Level::kError)
      << fmt::format("Error {} occurred because of '{}': {}",
                     static_cast<int>(error), reason, rd_kafka_err2str(error));

  if (error == RD_KAFKA_RESP_ERR__RESOLVE ||
      error == RD_KAFKA_RESP_ERR__TRANSPORT ||
      error == RD_KAFKA_RESP_ERR__AUTHENTICATION ||
      error == RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN) {
    ++stats_.connections_error;
  }
}

void ProducerImpl::LogCallback(const char* facility, const char* message,
                               int log_level) const {
  LOG(convertRdKafkaLogLevelToLoggingLevel(log_level))
      << logging::LogExtra{{{"kafka_callback", "log_callback"},
                            {"facility", facility}}}
      << message;
}

void ProducerImpl::DeliveryReportCallback(
    const rd_kafka_message_t* message) const {
  static constexpr utils::TrivialBiMap kMessageStatus{[](auto selector) {
    return selector()
        .Case(RD_KAFKA_MSG_STATUS_NOT_PERSISTED, "MSG_STATUS_NOT_PERSISTED")
        .Case(RD_KAFKA_MSG_STATUS_POSSIBLY_PERSISTED,
              "MSG_STATUS_POSSIBLY_PERSISTED")
        .Case(RD_KAFKA_MSG_STATUS_PERSISTED, "MSG_STATUS_PERSISTED");
  }};

  tracing::Span span{"delivery_report_callback"};
  span.AddTag("kafka_callback", "delivery_report_callback");

  const char* topic_name = rd_kafka_topic_name(message->rkt);

  auto* complete_handle = static_cast<DeliveryWaiter*>(message->_private);

  auto& topic_stats = stats_.topics_stats[topic_name];
  ++topic_stats->messages_counts.messages_total;

  const auto message_status = rd_kafka_message_status(message);
  DeliveryResult delivery_result{message->err, message_status};
  const auto message_latency_ms = GetMessageLatency(message);

  LOG_DEBUG() << fmt::format(
      "Message delivery report: err: {}, status: {}, latency: {}ms",
      rd_kafka_err2str(message->err),
      kMessageStatus.TryFind(message_status).value_or("<bad status>"),
      message_latency_ms.count());

  topic_stats->avg_ms_spent_time.GetCurrentCounter().Account(
      message_latency_ms.count());

  if (delivery_result.IsSuccess()) {
    ++topic_stats->messages_counts.messages_success;

    LOG_INFO() << fmt::format(
        "Message to topic '{}' delivered successfully to "
        "partition "
        "{} by offset {} in {}ms",
        topic_name, message->partition, message->offset,
        message_latency_ms.count());
  } else {
    ++topic_stats->messages_counts.messages_error;

    LOG_WARNING() << fmt::format("Failed to delivery message to topic '{}': {}",
                                 topic_name, rd_kafka_err2str(message->err));
  }

  complete_handle->SetDeliveryResult(std::move(delivery_result));
  delete complete_handle;
}

ProducerImpl::ProducerImpl(Configuration&& configuration)
    : delivery_timeout_(
          std::stoull(configuration.GetOption("delivery.timeout.ms"))),
      producer_(std::move(configuration).Release()) {
  /// Sets the callback which is called when delivery reports queue transfers
  /// from empty state to non-empty.
  /// Registered callback is called from internal librdkafka thread, not userver
  /// one.
  rd_kafka_queue_cb_event_enable(producer_.GetQueue(), &EventCallbackProxy,
                                 this);

  utils::PeriodicTask::Settings settings{std::chrono::seconds{1}};
  log_events_handler_.Start("kafka_producer_log_events_handler", settings,
                            [this] { HandleEvents("log events handler"); });
}

const Stats& ProducerImpl::GetStats() const { return stats_; }

DeliveryResult ProducerImpl::Send(
    const std::string& topic_name, std::string_view key,
    std::string_view message, std::optional<std::uint32_t> partition) const {
  LOG_INFO() << fmt::format("Message to topic '{}' is requested to send",
                            topic_name);
  auto delivery_result_future =
      ScheduleMessageDelivery(topic_name, key, message, partition);

  WaitUntilDeliveryReported(delivery_result_future);

  return delivery_result_future.get();
}

engine::Future<DeliveryResult> ProducerImpl::ScheduleMessageDelivery(
    const std::string& topic_name, std::string_view key,
    std::string_view message, std::optional<std::uint32_t> partition) const {
  auto waiter = std::make_unique<DeliveryWaiter>();
  auto wait_handle = waiter->GetFuture();

  /// `rd_kafka_producev` does not send given message. It only enqueues
  /// the message to the local queue to be send in future by `librdkafka`
  /// internal thread
  ///
  /// It uses pthread mutexes locks in its implementation, through must not be
  /// executed in main-task-processor
  ///
  /// 0 msgflags implies no message copying and freeing by
  /// `librdkafka` implementation. It is safe to pass actually a pointer
  /// to message data because it lives till the delivery callback is invoked
  /// @see
  /// https://github.com/confluentinc/librdkafka/blob/master/src/rdkafka.h#L4698
  /// for understanding of `msgflags` argument
  ///
  /// It is safe to release the `waiter` because (i)
  /// `rd_kafka_producev` does not throws, therefore it owns the `waiter`,
  /// (ii) delivery report callback fries its memory
  ///
  /// const qualifier remove for `message` is required because of
  /// the `librdkafka` API requirements. If `msgflags` set to
  /// `RD_KAFKA_MSG_F_FREE`, produce implementation fries the message
  /// data, though not const pointer is required

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-statement-expression"
#endif

  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks,cppcoreguidelines-pro-type-const-cast)
  const rd_kafka_resp_err_t enqueue_error = rd_kafka_producev(
      producer_.GetHandle(), RD_KAFKA_V_TOPIC(topic_name.c_str()),
      RD_KAFKA_V_KEY(key.data(), key.size()),
      RD_KAFKA_V_VALUE(const_cast<char*>(message.data()), message.size()),
      RD_KAFKA_V_MSGFLAGS(0),
      RD_KAFKA_V_PARTITION(partition.value_or(RD_KAFKA_PARTITION_UA)),
      RD_KAFKA_V_OPAQUE(waiter.get()), RD_KAFKA_V_END);
  // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks,cppcoreguidelines-pro-type-const-cast)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

  if (enqueue_error == RD_KAFKA_RESP_ERR_NO_ERROR) {
    [[maybe_unused]] auto _ = waiter.release();
  } else {
    LOG_WARNING() << fmt::format(
        "Failed to enqueue message to Kafka local queue: {}",
        rd_kafka_err2str(enqueue_error));
    waiter->SetDeliveryResult(DeliveryResult{enqueue_error});
  }

  return wait_handle;
}

EventHolder ProducerImpl::PollEvent() const {
  /// zero `timeout_ms` means no logical blocking wait for new events in
  /// producer queue. Actually, `rd_kafka_queue_poll` locks some pthread
  /// mutexes to access the queue state :*(
  ///
  /// `rd_kafka_queue_poll` is synchronized with `EventCallback` calls because
  /// librdkafka locks the same delivery queue both inside
  /// `rd_kafka_queue_poll` and when invokes event callback registered with
  /// `rd_kafka_queue_cb_event_enable`

  return EventHolder{
      rd_kafka_queue_poll(producer_.GetQueue(), /*timeout_ms=*/0)};
}

void ProducerImpl::DispatchEvent(const EventHolder& event_holder) const {
  UASSERT(event_holder);

  auto* event = event_holder.GetHandle();
  switch (rd_kafka_event_type(event)) {
    case RD_KAFKA_EVENT_DR: {
      const std::size_t message_count = rd_kafka_event_message_count(event);
      UASSERT_MSG(message_count > 0, "No messages in RD_KAFKA_EVENT_DR");
      LOG_DEBUG() << fmt::format("Delivery report event with {} messages",
                                 message_count);

      while (const auto* message = rd_kafka_event_message_next(event)) {
        DeliveryReportCallback(message);
      }
    } break;
    case RD_KAFKA_EVENT_ERROR: {
      ErrorCallback(rd_kafka_event_error(event),
                    rd_kafka_event_error_string(event),
                    rd_kafka_event_error_is_fatal(event));
    } break;
    case RD_KAFKA_EVENT_LOG: {
      const char* facility{nullptr};
      const char* message{nullptr};
      int log_level{};
      rd_kafka_event_log(event, &facility, &message, &log_level);
      LogCallback(facility, message, log_level);
    } break;
  }
}

std::size_t ProducerImpl::HandleEvents(std::string_view context) const {
  std::size_t handled{0};
  while (const EventHolder event = PollEvent()) {
    DispatchEvent(event);
    ++handled;
  }

  LOG_DEBUG() << fmt::format("Handled {} events ({})", handled, context);

  return handled;
}

void ProducerImpl::WaitUntilDeliveryReported(
    engine::Future<DeliveryResult>& delivery_result) const {
  /// While this task is waiting for corresponding message delivery, it can
  /// handle other messages delivery reports and errors.
  /// Waiting strategy is as follows:
  /// 1) If `delivery_result` is ready (means that current or another task
  /// called a DeliveryReport on the corresponding message), stop waiting and
  /// return from the function.
  /// 2) While there are events in the producer's queue, handle them and return
  /// to 1). If there are no event, proceed to 3).
  /// 3) No events are available, sleep until
  ///    [1] either `delivery_result` became ready (means that another task
  ///    handled that)
  ///    [2] or EventCallback signaled that there are events in
  ///    producer's queue
  /// After waking up, return to 1).
  ///
  /// But [1] and [2] in 3) are not synchronized and not mutually exclusive,
  /// though there is a race between them.
  /// To guarantee the waiting strategy progress the following invariant must be
  /// fullfield: if waiter was signaled from EventCallback, it must try to
  /// handle events from producer's queue.
  ///
  /// Remark: events are created by `librdkafka` internal threads, so it is not
  /// possible to atomically check queue emptyness and suspend the coroutine.

  while (!delivery_result.is_ready() && !engine::current_task::ShouldCancel()) {
    /// optimistic path. suppose that there are already some ready events from
    /// other tasks or our `delivery_result` is completed
    HandleEvents("optimistic path");

    if (delivery_result.is_ready()) {
      return;
    }

    EventWaiter waiter;
    /// (N) notice that in any time after waiter is pushed into waiters list and
    /// before it poped it may be waked up by EventCallback.
    /// Consequently, after waiter is poped from the list, it must try to handle
    /// the events if it was signaled (event `delivery_result` is ready).
    waiters_.PushWaiter(waiter);

    /// If there are events, it is not neccessary to sleep.
    if (HandleEvents("after waiter created, before sleep")) {
      waiters_.PopWaiter(waiter);
      if (waiter.event.IsReady()) {  // (N)
        LOG_DEBUG() << "Waiter were signaled before sleeping!";
        HandleEvents("after waiter poped, before sleep");
      }
      continue;
    }

    auto waked_up_by = engine::WaitAny(waiter.event, delivery_result);
    LOG_DEBUG() << fmt::format(
        "Wake up reason: {}",
        waked_up_by.has_value()
            ? (waked_up_by == 0 ? "EventCallback" : "DeliveryResult")
            : "Cancel");

    if (waked_up_by == 0) {
      UINVARIANT(waiter.event.IsReady(), "Waiter must be ready");

      /// we must try to handle events again, because it is possible that
      /// `delivery_result` is ready here and we are not going to the next
      /// while-loop cycle.
      HandleEvents("after waking up by EventCallback");
    } else if (waked_up_by == 1) {
      /// pop waiter, because other task completed the future.
      waiters_.PopWaiter(waiter);

      /// it is possible that EventCallback waked up current waiter after
      /// engine::WaitAny and before `PopWaiter` (equivalent to
      /// event.IsReady() == true).
      /// Therefore, so that the awakening does not turn out to be useless, we
      /// try to process the events again.
      if (waiter.event.IsReady()) {  // (N)
        HandleEvents("after waking up by DeliveryResult");
      }
      break;
    } else {
      waiters_.PopWaiter(waiter);
      /// waitAny is canceled, but delivery reports are handled in another tasks
      /// or/and in producer's dctor.

      LOG_WARNING() << "Delivery waiting loop is canceled before the message "
                       "is delivered. Ensure to call a producer destructor to "
                       "guarantee that all messages are delivered";
      break;
    }
  }
}

void ProducerImpl::EventCallback() {
  /// The callback is called from internal librdkafka thread, i.e. not in
  /// coroutine environment, therefore not all synchronization
  /// primitives can be used in the callback body.

  LOG_INFO()
      << "Producer events queue became non-empty. Waking up event waiter";
  waiters_.PopAndWakeupOne();
}

void ProducerImpl::WaitUntilAllMessagesDelivered() && {
  static constexpr std::size_t kFlushSteps{10};

  log_events_handler_.Stop();
  /// unregister EventCallback
  rd_kafka_queue_cb_event_enable(producer_.GetQueue(), nullptr, nullptr);
  waiters_.WakeupAll();

  const auto flush_step_duration = 2 * delivery_timeout_ / kFlushSteps;
  for (std::size_t step{1}; step <= kFlushSteps; ++step) {
    /// `rd_kafka_flush` returns true if no messages are waiting for the
    /// delivery
    if (rd_kafka_flush(producer_.GetHandle(), /*timeout_ms=*/0) ==
        RD_KAFKA_RESP_ERR_NO_ERROR) {
      LOG_INFO() << "All messages are successfully delivered";
      break;
    }
    engine::SleepFor(flush_step_duration);

    HandleEvents("waiting until all messages delivered");

    if (step < kFlushSteps) {
      LOG_WARNING() << fmt::format(
          "[retry {}] Producer flushing timeouted on producer destroy. Waiting "
          "more..",
          step);
    } else {
      LOG_ERROR() << fmt::format(
          "Some producer messages are probably not delivered :(");
    }
  }
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
