#include "channel_pool.hpp"

#include <userver/engine/task/task.hpp>
#include <userver/utils/assert.hpp>

#include <urabbitmq/channel_ptr.hpp>
#include <urabbitmq/impl/amqp_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ChannelPool::ChannelPool(clients::dns::Resolver& resolver,
                         const ChannelPoolSettings& settings)
    : handler_{resolver,
               engine::current_task::GetEventThread(),
               {"amqp://guest:guest@localhost/"}},
      conn_{handler_},
      settings_{settings},
      queue_{settings.max_channels} {
  for (size_t i = 0; i < settings_.max_channels; ++i) {
    AddChannel();
  }
}

ChannelPool::~ChannelPool() {
  while (true) {
    auto* ptr = TryPop();
    if (!ptr) break;

    Drop(ptr);
  }
}

ChannelPtr ChannelPool::Acquire() { return {shared_from_this(), Pop()}; }

void ChannelPool::Release(impl::IAmqpChannel* channel) {
  UASSERT(channel);

  if (!queue_.bounded_push(channel)) {
    Drop(channel);
  }
}

impl::IAmqpChannel* ChannelPool::Pop() {
  auto* ptr = TryPop();

  if (!ptr) {
    throw std::runtime_error{"oh well"};
  }
  return ptr;
}

impl::IAmqpChannel* ChannelPool::TryPop() {
  impl::IAmqpChannel* ptr{nullptr};
  if (queue_.pop(ptr)) {
    return ptr;
  }

  return nullptr;
}

std::unique_ptr<impl::IAmqpChannel> ChannelPool::Create() {
  switch (settings_.mode) {
    case ChannelPoolMode::kUnreliable:
      return std::make_unique<impl::AmqpChannel>(conn_);
    case ChannelPoolMode::kReliable:
      return std::make_unique<impl::AmqpReliableChannel>(conn_);
  }
}

void ChannelPool::Drop(impl::IAmqpChannel* channel) {
  std::default_delete<impl::IAmqpChannel>{}(channel);
}

void ChannelPool::AddChannel() {
  auto* ptr = Create().release();
  if (!queue_.bounded_push(ptr)) {
    Drop(ptr);
  }
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
