#include "consumer_base_impl.hpp"

#include <string>

#include <fmt/format.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>

#include <urabbitmq/connection.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>
#include <urabbitmq/impl/deferred_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

constexpr std::chrono::milliseconds kSetQosTimeout{2000};
constexpr std::chrono::milliseconds kStartTimeout{2000};

}  // namespace

ConsumerBaseImpl::ConsumerBaseImpl(ConnectionPtr&& connection,
                                   const ConsumerSettings& settings)
    : dispatcher_{engine::current_task::GetTaskProcessor()},
      queue_name_{settings.queue.GetUnderlying()},
      connection_ptr_{std::move(connection)},
      channel_{connection_ptr_->GetChannel()} {
  // We take ownership of the connection, because if it remains pooled
  // things get messy with lifetimes and callbacks
  connection_ptr_.Adopt();

  channel_.SetQos(settings.prefetch_count,
                  engine::Deadline::FromDuration(kSetQosTimeout));
}

ConsumerBaseImpl::~ConsumerBaseImpl() { Stop(); }

void ConsumerBaseImpl::Start(DispatchCallback cb) {
  if (started_) {
    throw std::logic_error{"Consumer is already started."};
  }
  if (stopped_) {
    throw std::logic_error{"Consumer has been explicitly stopped."};
  }
  dispatch_callback_ = std::move(cb);

  LOG_INFO() << "Starting a consumer for '" << queue_name_ << "' queue";

  channel_.SetupConsumer(
      queue_name_,
      // error callback
      [this](const char*) { broken_.store(true, std::memory_order_relaxed); },
      // success callback
      [this](const std::string& consumer_tag) {
        if (!stopped_) {
          consumer_tag_.emplace(consumer_tag);
        }
      },
      // message callback
      [this](const AMQP::Message& message, uint64_t delivery_tag, bool) {
        // We received a message but won't ack it, so it will be requeued
        // at some point
        if (!stopped_) {
          OnMessage(message, delivery_tag);
        }
      },
      engine::Deadline::FromDuration(kStartTimeout));

  started_ = true;

  LOG_INFO() << "Started a consumer for '" << queue_name_ << "' queue";
}

void ConsumerBaseImpl::Stop() {
  if (!started_ || stopped_) return;

  stopped_ = true;
  try {
    channel_.CancelConsumer(consumer_tag_);
  } catch (const std::exception&) {
    // Connection is broken, but that's not a problem
  }

  // Cancel all the active dispatched tasks
  bts_->CancelAndWait();

  // Destroy the connection: at this point all the remaining tasks are stopped,
  // consumer is either stopped or in unknown state - that could happen if we
  // didn't receive onSuccess callback yet.
  // I'm not sure whether consumer.onMessage could fire in channel destructor,
  // so we guard against that via `stopped_`
  { [[maybe_unused]] ConnectionPtr tmp{std::move(connection_ptr_)}; }
}

bool ConsumerBaseImpl::IsBroken() const {
  return broken_ || !connection_ptr_.IsUsable();
}

void ConsumerBaseImpl::OnMessage(const AMQP::Message& message,
                                 uint64_t delivery_tag) {
  std::string span_name{fmt::format("consume_{}_{}", queue_name_,
                                    consumer_tag_.value_or("ctag:unknown"))};
  std::string trace_id = message.headers().get("u-trace-id");
  std::string message_data{message.body(), message.bodySize()};

  bts_->Detach(engine::AsyncNoSpan(
      dispatcher_, [this, message = std::move(message_data),
                    span_name = std::move(span_name),
                    trace_id = std::move(trace_id), delivery_tag]() mutable {
        auto span = tracing::Span::MakeSpan(std::move(span_name), trace_id, {});

        bool success = false;
        try {
          dispatch_callback_(std::move(message));
          success = true;
        } catch (const std::exception& ex) {
          LOG_ERROR() << "Failed to process the consumed message, " << ex.what()
                      << "; would requeue";
        }

        try {
          if (success) {
            channel_.Ack(delivery_tag, {});
            channel_.AccountMessageConsumed();
          } else {
            channel_.Reject(delivery_tag, true, {});
          }
        } catch (const std::exception& ex) {
          LOG_WARNING()
              << "Failed to " << (success ? "ack" : "requeue")
              << " the message, it will be requeued by RabbitMQ at some point";
        }
      }));
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
