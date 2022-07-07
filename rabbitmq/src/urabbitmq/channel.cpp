#include <userver/urabbitmq/channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Channel::Impl final {};

Channel::Channel() : impl_{std::make_unique<Impl>()} {}

Channel::~Channel() = default;

void Channel::Publish(const Exchange& exchange, const std::string& message) {}

void Channel::PublishReliable(const Exchange& exchange,
                              const std::string& message) {}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
