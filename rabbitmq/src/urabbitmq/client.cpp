#include <userver/urabbitmq/client.hpp>

#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/client_impl.hpp>
#include <urabbitmq/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Client::Client(clients::dns::Resolver& resolver, const ClientSettings& settings)
    : impl_{resolver, settings} {}

Client::~Client() = default;

AdminChannel Client::GetAdminChannel() {
  return {shared_from_this(), impl_->GetUnreliable()};
}

Channel Client::GetChannel() {
  return {shared_from_this(), impl_->GetUnreliable(), impl_->GetReliable()};
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
