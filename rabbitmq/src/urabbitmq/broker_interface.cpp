#include <userver/urabbitmq/broker_interface.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

IAdminInterface::~IAdminInterface() = default;

IChannelInterface::~IChannelInterface() = default;

IReliableChannelInterface::~IReliableChannelInterface() = default;

}  // namespace urabbitmq

USERVER_NAMESPACE_END
