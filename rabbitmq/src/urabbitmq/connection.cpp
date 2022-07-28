#include "connection.hpp"

#include <urabbitmq/channel_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Connection::Connection(clients::dns::Resolver& resolver,
                       engine::ev::ThreadControl& thread,
                       const EndpointInfo& endpoint,
                       const AuthSettings& auth_settings, size_t max_channels)
    : handler_{resolver, thread, endpoint, auth_settings},
      connection_{handler_},
      channels_{ChannelPool::Create(handler_, connection_,
                                    ChannelPool::ChannelMode::kDefault,
                                    max_channels)},
      reliable_channels_{ChannelPool::Create(
          handler_, connection_, ChannelPool::ChannelMode::kReliable,
          max_channels)} {}

Connection::~Connection() {
  // Pooled channels might outlive us, and since they hold shared_ptr<Pool>
  // we have to ensure that pools stop using our resources (namely, connection)

  channels_->Stop();
  reliable_channels_->Stop();
}

ChannelPtr Connection::Acquire() const { return channels_->Acquire(); }

ChannelPtr Connection::AcquireReliable() const {
  return reliable_channels_->Acquire();
}

bool Connection::IsBroken() const { return handler_.IsBroken(); }

}  // namespace urabbitmq

USERVER_NAMESPACE_END
