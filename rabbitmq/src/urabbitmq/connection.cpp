#include "connection.hpp"

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Connection::Connection(clients::dns::Resolver& resolver,
                       const EndpointInfo& endpoint,
                       const AuthSettings& auth_settings,
                       bool secure,
                       statistics::ConnectionStatistics& stats,
                       engine::Deadline deadline)
    : handler_{resolver, endpoint, auth_settings,
               secure, stats, deadline},
      connection_{handler_, deadline},
      channel_{connection_, deadline},
      reliable_channel_{connection_, deadline} {}

Connection::~Connection() = default;

impl::AmqpChannel& Connection::GetChannel() { return channel_; }

impl::AmqpReliableChannel& Connection::GetReliableChannel() {
  return reliable_channel_;
}

void Connection::ResetCallbacks() {
  channel_.ResetCallbacks();
  reliable_channel_.ResetCallbacks();
}

bool Connection::IsBroken() const { return handler_.IsBroken(); }

}  // namespace urabbitmq

USERVER_NAMESPACE_END
