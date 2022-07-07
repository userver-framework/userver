#pragma once

#include <string>

#include <userver/urabbitmq/exchange_type.hpp>
#include <userver/urabbitmq/typedefs.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class AdminChannel final {
 public:
  void DeclareExchange(const Exchange& exchange, ExchangeType type);

  void DeclareQueue(const Queue& queue);

  void BindQueue(const Exchange& exchange, const Queue& queue,
                 const std::string& routing_key);
 private:
};

}

USERVER_NAMESPACE_END