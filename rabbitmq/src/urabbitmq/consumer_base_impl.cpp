#include "consumer_base_impl.hpp"

#include <string>

#include <fmt/format.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>

#include <urabbitmq/impl/amqp_channel.hpp>
#include <urabbitmq/impl/deferred_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ConsumerBaseImpl::ConsumerBaseImpl(ChannelPtr&& channel,
                                   const std::string& queue)
    : channel_ptr_{std::move(channel)},
      channel_{dynamic_cast<impl::AmqpChannel*>(channel_ptr_.Get())},
      dispatcher_{engine::current_task::GetTaskProcessor()},
      queue_name_{queue},
      alive_{std::make_shared<bool>(true)} {
  if (!channel_) {
    throw std::runtime_error{
        "Shouldn't happen, consumer shouldn't be crated on a reliable channel"};
  }

  auto deferred = impl::DeferredWrapper::Create();

  channel_->GetEvThread().RunInEvLoopSync(
      [this, deferred] { deferred->Wrap(channel_->channel_->setQos(200)); });

  deferred->Wait();
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

  channel_->GetEvThread().RunInEvLoopSync([this] {
    channel_->channel_->onError([this](const char*) { broken_ = true; });
    channel_->channel_->consume(queue_name_)
        .onSuccess([alive = alive_, this](const std::string& consumer_tag) {
          if (*alive_) {
            consumer_tag_.emplace(consumer_tag);
          }
        })
        .onMessage([alive = alive_, this](const AMQP::Message& message,
                                          uint64_t delivery_tag, bool) {
          if (*alive_) {
            OnMessage(message, delivery_tag);
          }
        });
  });
  started_ = true;
}

void ConsumerBaseImpl::Stop() {
  if (stopped_) return;

  channel_->GetEvThread().RunInEvLoopSync([this] {
    *alive_ = false;
    if (consumer_tag_.has_value()) {
      channel_->Cancel(*consumer_tag_);
    }
  });
  bts_->CancelAndWait();
  stopped_ = true;
}

bool ConsumerBaseImpl::IsBroken() const { return broken_; }

void ConsumerBaseImpl::OnMessage(const AMQP::Message& message,
                                 uint64_t delivery_tag) {
  UASSERT(channel_->GetEvThread().IsInEvThread());

  std::string span_name{fmt::format("consume_{}", queue_name_)};
  std::string message_data{message.body(), message.bodySize()};

  bts_->Detach(engine::AsyncNoSpan(
      dispatcher_, [this, message = std::move(message_data),
                    span_name = std::move(span_name), delivery_tag]() mutable {
        tracing::Span span{std::move(span_name)};

        bool success = false;
        try {
          dispatch_callback_(std::move(message));
          success = true;
        } catch (const std::exception& ex) {
          LOG_ERROR() << "Failed to process the consumed message, " << ex.what()
                      << "; would requeue";
        }

        if (success)
          channel_->Ack(delivery_tag);
        else
          channel_->Reject(delivery_tag, true);
      }));
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
