#include "channel_ptr.hpp"

#include <urabbitmq/impl/amqp_channel.hpp>

#include <urabbitmq/channel_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ChannelPtr::ChannelPtr(std::shared_ptr<ChannelPool>&& pool, impl::IAmqpChannel* channel)
: pool_{std::move(pool)}, channel_{channel} {}

ChannelPtr::~ChannelPtr() {
  Release();
}

ChannelPtr::ChannelPtr(ChannelPtr&& other) {
  Release();
  pool_ = std::move(other.pool_);
  channel_ = std::move(other.channel_);
}

impl::IAmqpChannel& ChannelPtr::operator*() const {
  return *channel_;
}

impl::IAmqpChannel* ChannelPtr::operator->() const noexcept {
  return channel_.get();
}

void ChannelPtr::Release() {
  if (!channel_) return;

  pool_->Release(channel_.release());
}

}

USERVER_NAMESPACE_END
