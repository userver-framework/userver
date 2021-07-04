#pragma once

#include <server/net/listener_config.hpp>
#include <userver/engine/io/socket.hpp>

namespace server::net {

engine::io::Socket CreateSocket(const ListenerConfig& config);

}  // namespace server::net
