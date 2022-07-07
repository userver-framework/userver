#include <userver/urabbitmq/admin_channel.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

void AdminChannel::DeclareExchange(const Exchange& exchange,
                                   ExchangeType type) {}

void AdminChannel::DeclareQueue(const Queue& queue) {}

void AdminChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                             const std::string& routing_key) {}

}  // namespace urabbitmq

USERVER_NAMESPACE_END