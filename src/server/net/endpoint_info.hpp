#pragma once

#include <atomic>

#include <server/request_handlers/request_handlers.hpp>

#include "listener_config.hpp"

namespace server {
namespace net {

struct EndpointInfo {
  EndpointInfo(const ListenerConfig&, RequestHandlers&);

  const ListenerConfig& listener_config;
  RequestHandlers& request_handlers;

  std::atomic<size_t> connection_count;
};

}  // namespace net
}  // namespace server
