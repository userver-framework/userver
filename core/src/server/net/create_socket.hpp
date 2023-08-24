#pragma once

#include <server/net/listener_config.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

engine::io::Socket CreateSocket(const ListenerConfig& config);

}  // namespace server::net

USERVER_NAMESPACE_END
