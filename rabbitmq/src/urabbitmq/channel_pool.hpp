#pragma once

#include <memory>

#include <boost/lockfree/queue.hpp>

#include <urabbitmq/channel_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace impl {
class IAmqpChannel;
class AmqpConnection;
}  // namespace impl

enum class ChannelPoolMode {
  // Default mode for a channel, Publish is 'fire and forget'
  kUnreliable,
  // Publisher Confirms, Publish requires explicit ack from remote
  kReliable
};

struct ChannelPoolSettings final {
  ChannelPoolMode mode;
  size_t max_channels;
};

class ChannelPool final : public std::enable_shared_from_this<ChannelPool> {
 public:
  ChannelPool(impl::AmqpConnection& conn, const ChannelPoolSettings& settings);
  ~ChannelPool();

  ChannelPtr Acquire();
  void Release(impl::IAmqpChannel* channel);

 private:
  impl::IAmqpChannel* Pop();
  impl::IAmqpChannel* TryPop();

  std::unique_ptr<impl::IAmqpChannel> Create();
  static void Drop(impl::IAmqpChannel* channel);

  void AddChannel();

  impl::AmqpConnection& conn_;
  ChannelPoolSettings settings_;
  boost::lockfree::queue<impl::IAmqpChannel*> queue_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END