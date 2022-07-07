#include "consumer_base_impl.hpp"

#include <string>

#include <fmt/format.h>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/tracing/span.hpp>

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
    throw std::runtime_error{"Shouldn't happen, consumer shouldn't be crated on a reliable channel"};
  }

  auto deferred = impl::DeferredWrapper::Create();

  channel_->GetEvThread().RunInEvLoopSync(
      [this, deferred] { deferred->Wrap(channel_->channel_->setQos(10)); });

  deferred->Wait();
}

ConsumerBaseImpl::~ConsumerBaseImpl() { Stop(); }

void ConsumerBaseImpl::Start(DispatchCallback cb) {
  dispatch_callback_ = std::move(cb);

  channel_->GetEvThread().RunInEvLoopSync([this] {
    channel_->channel_->consume(queue_name_)
        .onSuccess([alive = alive_, this](const std::string& consumer_tag) {
          if (alive_) {
            consumer_tag_.emplace(consumer_tag);
          }
        })
        .onMessage([alive = alive_, this](const AMQP::Message& message,
                                          uint64_t delivery_tag, bool) {
          if (alive_) {
            OnMessage(message, delivery_tag);
          }
        });
  });
}

void ConsumerBaseImpl::Stop() {
  if (stopped_) return;

  stopped_ = true;
  channel_->GetEvThread().RunInEvLoopSync([this] {
    *alive_ = false;
    if (consumer_tag_.has_value()) {
      channel_->channel_->cancel(*consumer_tag_);
    }
  });
  const auto a = bts_->ActiveTasksApprox();
  bts_->CancelAndWait();
}

void ConsumerBaseImpl::OnMessage(const AMQP::Message& message,
                                 uint64_t delivery_tag) {
  UASSERT(channel_->GetEvThread().IsInEvThread());

  std::string span_name{fmt::format("consume_{}", queue_name_)};
  std::string message_data{message.body(), message.bodySize()};

  // TODO : engine::AsyncNoSpan -> utils::Async
  // https://github.com/userver-framework/userver/issues/48
  bts_->Detach(engine::AsyncNoSpan(
      dispatcher_, [this, message = std::move(message_data),
                    span_name = std::move(span_name), delivery_tag]() mutable {
        tracing::Span span{std::move(span_name)};
        dispatch_callback_(std::move(message));

        channel_->GetEvThread().RunInEvLoopAsync(
            [this, delivery_tag] { channel_->channel_->ack(delivery_tag); });
      }));
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
