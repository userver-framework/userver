#pragma once

#include <userver/clients/dns/resolver_fwd.hpp>
#include <urabbitmq/channel_ptr.hpp>

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

}

USERVER_NAMESPACE_END
