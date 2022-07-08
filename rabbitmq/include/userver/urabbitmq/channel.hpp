#pragma once

#include <string>

#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Cluster;
class ChannelPtr;

class Channel final {
 public:
  Channel(std::shared_ptr<Cluster>&& cluster, ChannelPtr&& channel,
          ChannelPtr&& reliable_channel);
  ~Channel();

  void Publish(const Exchange& exchange, const std::string& routing_key,
               const std::string& message);

  void PublishReliable(const Exchange& exchange, const std::string& routing_key,
                       const std::string& message);

 private:
  std::shared_ptr<Cluster> cluster_;

  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END