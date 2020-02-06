#pragma once

#include <engine/io/socket.hpp>
#include <server/net/listener_config.hpp>

namespace server::net {

engine::io::Socket CreateSocket(const ListenerConfig& config);

}  // namespace server::net
