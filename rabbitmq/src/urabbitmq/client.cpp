#include <userver/urabbitmq/client.hpp>

#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/channel.hpp>

#include <urabbitmq/channel_pool.hpp>
#include <urabbitmq/client_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Client::Client(clients::dns::Resolver& resolver) : impl_{resolver} {}

Client::~Client() = default;

AdminChannel Client::GetAdminChannel() {
  return AdminChannel{shared_from_this(), impl_->GetUnreliable()};
}

Channel Client::GetChannel() {
  return Channel{shared_from_this(), impl_->GetUnreliable(),
                 impl_->GetReliable()};
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
