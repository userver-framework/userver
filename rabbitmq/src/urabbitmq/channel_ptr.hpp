#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class IAmqpChannel;
}

class Connection;

class ChannelPtr final {
 public:
  ChannelPtr(std::shared_ptr<Connection>&& connection,
             impl::IAmqpChannel* channel);
  ~ChannelPtr();

  ChannelPtr(ChannelPtr&& other) noexcept;

  impl::IAmqpChannel* Get() const;

  impl::IAmqpChannel& operator*() const;
  impl::IAmqpChannel* operator->() const noexcept;

 private:
  void Release() noexcept;

  std::shared_ptr<Connection> connection_;
  std::unique_ptr<impl::IAmqpChannel> channel_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
