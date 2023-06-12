#include <userver/urabbitmq/client.hpp>

#include <userver/formats/json/value.hpp>

#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/client_impl.hpp>
#include <urabbitmq/connection_helper.hpp>
#include <urabbitmq/connection_ptr.hpp>
#include <urabbitmq/make_shared_enabler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

std::shared_ptr<Client> Client::Create(clients::dns::Resolver& resolver,
                                       const ClientSettings& settings) {
  return std::make_shared<MakeSharedEnabler<Client>>(resolver, settings);
}

Client::Client(clients::dns::Resolver& resolver, const ClientSettings& settings)
    : impl_{resolver, settings} {}

Client::~Client() = default;

void Client::DeclareExchange(const Exchange& exchange, Exchange::Type type,
                             utils::Flags<Exchange::Flags> flags,
                             engine::Deadline deadline) {
  auto awaiter = ConnectionHelper::DeclareExchange(
      impl_->GetConnection(deadline), exchange, type, flags, deadline);
  awaiter.Wait(deadline);
}

void Client::DeclareQueue(const Queue& queue, utils::Flags<Queue::Flags> flags,
                          engine::Deadline deadline) {
  auto awaiter = ConnectionHelper::DeclareQueue(impl_->GetConnection(deadline),
                                                queue, flags, deadline);
  awaiter.Wait(deadline);
}

void Client::BindQueue(const Exchange& exchange, const Queue& queue,
                       const std::string& routing_key,
                       engine::Deadline deadline) {
  auto awaiter = ConnectionHelper::BindQueue(
      impl_->GetConnection(deadline), exchange, queue, routing_key, deadline);
  awaiter.Wait(deadline);
}

void Client::RemoveExchange(const Exchange& exchange,
                            engine::Deadline deadline) {
  auto awaiter = ConnectionHelper::RemoveExchange(
      impl_->GetConnection(deadline), exchange, deadline);
  awaiter.Wait(deadline);
}

void Client::RemoveQueue(const Queue& queue, engine::Deadline deadline) {
  auto awaiter = ConnectionHelper::RemoveQueue(impl_->GetConnection(deadline),
                                               queue, deadline);
  awaiter.Wait(deadline);
}

void Client::Publish(const Exchange& exchange, const std::string& routing_key,
                     const std::string& message, MessageType type,
                     engine::Deadline deadline) {
  ConnectionHelper::Publish(impl_->GetConnection(deadline), exchange,
                            routing_key, message, type, deadline);
}

void Client::PublishReliable(const Exchange& exchange,
                             const std::string& routing_key,
                             const std::string& message, MessageType type,
                             engine::Deadline deadline) {
  auto awaiter = ConnectionHelper::PublishReliable(
      impl_->GetConnection(deadline), exchange, routing_key, message, type,
      deadline);
  awaiter.Wait(deadline);
}

AdminChannel Client::GetAdminChannel(engine::Deadline deadline) {
  return {impl_->GetConnection(deadline)};
}

Channel Client::GetChannel(engine::Deadline deadline) {
  return {impl_->GetConnection(deadline)};
}

ReliableChannel Client::GetReliableChannel(engine::Deadline deadline) {
  return {impl_->GetConnection(deadline)};
}

void Client::WriteStatistics(utils::statistics::Writer& writer) const {
  return impl_->WriteStatistics(writer);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
