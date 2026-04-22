#pragma once

/// @file userver/net/detail/connect_tcp_to_addrs.hpp
/// @brief Internal helper: connect to one of the resolved addresses

#include <cstdint>
#include <string_view>
#include <vector>

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace net::detail {

/// @throws engine::io::IoException "engine::io::IoException" if connection to all addresses fails
engine::io::Socket ConnectTcpToAddrs(
    std::string_view host,
    std::uint16_t port,
    const std::vector<engine::io::Sockaddr>& addrs,
    engine::Deadline deadline
);

}  // namespace net::detail

USERVER_NAMESPACE_END
