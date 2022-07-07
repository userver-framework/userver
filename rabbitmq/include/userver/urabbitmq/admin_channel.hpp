#pragma once

#include <string>
#include <memory>

#include <userver/urabbitmq/exchange_type.hpp>
#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class Cluster;
class ChannelPtr;

class AdminChannel final {
 public:
  AdminChannel(std::shared_ptr<Cluster>&& cluster, ChannelPtr&& channel);
  ~AdminChannel();

  void DeclareExchange(const Exchange& exchange, ExchangeType type);

  void DeclareQueue(const Queue& queue);

  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key);

 private:
  std::shared_ptr<Cluster> cluster_;

  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END