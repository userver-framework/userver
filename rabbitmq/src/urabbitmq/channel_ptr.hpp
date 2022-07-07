#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class IAmqpChannel;
}

class ChannelPool;

class ChannelPtr final {
 public:
  ChannelPtr(std::shared_ptr<ChannelPool>&& pool, impl::IAmqpChannel* channel);
  ~ChannelPtr();

  ChannelPtr(ChannelPtr&& other);

  impl::IAmqpChannel& operator*() const;
  impl::IAmqpChannel* operator->() const noexcept;

 private:
  void Release();

  std::shared_ptr<ChannelPool> pool_;
  std::unique_ptr<impl::IAmqpChannel> channel_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
