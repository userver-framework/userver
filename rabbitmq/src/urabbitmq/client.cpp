#include <userver/urabbitmq/client.hpp>

#include <userver/formats/json/value.hpp>

#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/client_impl.hpp>
#include <urabbitmq/connection.hpp>
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

AdminChannel Client::GetAdminChannel() { return {impl_->GetUnreliable()}; }

Channel Client::GetChannel() { return {impl_->GetUnreliable()}; }

ReliableChannel Client::GetReliableChannel() { return {impl_->GetReliable()}; }

formats::json::Value Client::GetStatistics() const {
  return impl_->GetStatistics();
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
