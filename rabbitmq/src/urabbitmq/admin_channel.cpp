#include <userver/urabbitmq/admin_channel.hpp>

#include <urabbitmq/connection_helper.hpp>
#include <urabbitmq/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

AdminChannel::AdminChannel(ConnectionPtr&& channel)
    : impl_{std::move(channel)} {}

AdminChannel::~AdminChannel() = default;

AdminChannel::AdminChannel(AdminChannel&& other) noexcept = default;

void AdminChannel::DeclareExchange(const Exchange& exchange,
                                   Exchange::Type type,
                                   utils::Flags<Exchange::Flags> flags,
                                   engine::Deadline deadline) {
  ConnectionHelper::DeclareExchange(*impl_, exchange, type, flags, deadline)
      .Wait(deadline);
}

void AdminChannel::DeclareQueue(const Queue& queue,
                                utils::Flags<Queue::Flags> flags,
                                engine::Deadline deadline) {
  ConnectionHelper::DeclareQueue(*impl_, queue, flags, deadline).Wait(deadline);
}

void AdminChannel::BindQueue(const Exchange& exchange, const Queue& queue,
                             const std::string& routing_key,
                             engine::Deadline deadline) {
  ConnectionHelper::BindQueue(*impl_, exchange, queue, routing_key, deadline)
      .Wait(deadline);
}

void AdminChannel::RemoveExchange(const Exchange& exchange,
                                  engine::Deadline deadline) {
  ConnectionHelper::RemoveExchange(*impl_, exchange, deadline).Wait(deadline);
}

void AdminChannel::RemoveQueue(const Queue& queue, engine::Deadline deadline) {
  ConnectionHelper::RemoveQueue(*impl_, queue, deadline).Wait(deadline);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
