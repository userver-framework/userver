#include "channel_ptr.hpp"

#include <urabbitmq/impl/amqp_channel.hpp>

#include <urabbitmq/channel_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ChannelPtr::ChannelPtr(std::shared_ptr<ChannelPool>&& pool,
                       impl::IAmqpChannel* channel)
    : pool_{std::move(pool)}, channel_{channel} {
  UINVARIANT(!channel_->Broken(), "oops");
}

ChannelPtr::~ChannelPtr() { Release(); }

ChannelPtr::ChannelPtr(ChannelPtr&& other) noexcept
    : pool_{std::move(other.pool_)},
      channel_{std::move(other.channel_)},
      should_return_to_pool_{other.should_return_to_pool_} {}

impl::IAmqpChannel* ChannelPtr::Get() const {
  if (pool_->IsBroken()) {
    throw std::runtime_error{"Connection is broken"};
  }
  if (pool_->IsBlocked()) {
    // We don't wait for pool to become writeable and throw right away instead
    // to avoid unexpected stalling and prevent system overload
    throw std::runtime_error{
        "Driver or Broker can't keep up with write ratio, retry later"};
  }
  if (channel_->Broken()) {
    throw std::runtime_error{"Channel is broken"};
  }
  return channel_.get();
}

void ChannelPtr::Adopt() {
  pool_->NotifyChannelAdopted();
  should_return_to_pool_ = false;
}

void ChannelPtr::Release() noexcept {
  if (!channel_) {
    return;
  }

  if (!should_return_to_pool_) {
    pool_->NotifyChannelClosed();
    return;
  }

  pool_->Release(std::move(channel_));
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
