#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class IAmqpChannel;
}

class ChannelPool;
class ConsumerBaseImpl;

class ChannelPtr final {
 public:
  ChannelPtr(std::shared_ptr<ChannelPool>&& pool, impl::IAmqpChannel* channel);
  ~ChannelPtr();

  ChannelPtr(ChannelPtr&& other) noexcept;

  impl::IAmqpChannel* Get() const;

 private:
  friend class ConsumerBaseImpl;
  void Adopt();

  void Release() noexcept;

  std::shared_ptr<ChannelPool> pool_;
  std::unique_ptr<impl::IAmqpChannel> channel_;

  bool should_return_to_pool_{true};
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
