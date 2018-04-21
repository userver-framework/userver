#pragma once

#include <atomic>

#include <server/request_handling/request_handler.hpp>

#include "listener_config.hpp"

namespace server {
namespace net {

struct EndpointInfo {
  EndpointInfo(const ListenerConfig&, request_handling::RequestHandler&);

  const ListenerConfig& listener_config;
  request_handling::RequestHandler& request_handler;

  std::atomic<size_t> connection_count;
};

}  // namespace net
}  // namespace server
