#pragma once

/// @file userver/net/connect_tcp_by_name.hpp
/// @brief @copybrief net::ConnectTcpByName

#include <cstdint>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace net {

/// @brief Establishes a TCP connection to the given host and port using DNS
/// resolution via the provided @ref clients::dns::Resolver.
///
/// Resolves the host name using \p resolver (which may use cache, /etc/hosts,
/// and network DNS), then tries to connect to each resolved address in turn.
/// The first successful connection is returned. TCP_NODELAY is set on the
/// socket.
///
/// @param host DNS name or IP address to connect to (e.g. "localhost",
/// "example.com", "127.0.0.1").
/// @param port TCP port number.
/// @param resolver DNS resolver instance (e.g. from @ref clients::dns::Component).
/// @param deadline Maximum time allowed for the whole operation (resolution +
/// connection). Same deadline is used for both resolve and connect steps.
/// @return A connected TCP socket.
/// @throws clients::dns::ResolverException or @ref clients::dns::NotResolvedException
/// if the host name cannot be resolved.
/// @throws engine::io::IoException if connection to all resolved addresses
/// fails (e.g. connection refused, timeout).
///
/// @note Without a Resolver (e.g. in non-component code) use
/// @ref net::blocking::ConnectTcpByName.
///
/// @snippet src/net/connect_tcp_by_name_test.cpp ConnectTcpByName localhost
engine::io::Socket ConnectTcpByName(
    const std::string& host,
    std::uint16_t port,
    clients::dns::Resolver& resolver,
    engine::Deadline deadline
);

}  // namespace net

USERVER_NAMESPACE_END
