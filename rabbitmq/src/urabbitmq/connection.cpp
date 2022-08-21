#include "connection.hpp"

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

Connection::Connection(clients::dns::Resolver& resolver,
                       const EndpointInfo& endpoint,
                       const AuthSettings& auth_settings,
                       size_t max_in_flight_requests, bool secure,
                       statistics::ConnectionStatistics& stats,
                       engine::Deadline deadline)
    : handler_{resolver, endpoint, auth_settings, secure, stats, deadline},
      connection_{handler_, max_in_flight_requests, deadline},
      channel_{connection_},
      reliable_channel_{connection_} {}

Connection::~Connection() = default;

impl::AmqpChannel& Connection::GetChannel() { return channel_; }

impl::AmqpReliableChannel& Connection::GetReliableChannel() {
  return reliable_channel_;
}

bool Connection::IsBroken() const { return handler_.IsBroken(); }

void Connection::EnsureUsable() const {
  if (IsBroken()) {
    throw std::runtime_error{"Connection is broken"};
  }
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
