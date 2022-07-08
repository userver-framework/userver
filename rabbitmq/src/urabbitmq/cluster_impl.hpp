#pragma once

#include <urabbitmq/channel_ptr.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ClusterImpl final {
 public:
  ClusterImpl(clients::dns::Resolver& resolver);

  ChannelPtr GetUnreliable();

  ChannelPtr GetReliable();

 private:
  std::shared_ptr<ChannelPool> unreliable_;
  std::shared_ptr<ChannelPool> reliable_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
