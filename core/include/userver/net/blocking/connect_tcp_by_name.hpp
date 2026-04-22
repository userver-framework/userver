#pragma once

/// @file userver/net/blocking/connect_tcp_by_name.hpp
/// @brief @copybrief net::blocking::ConnectTcpByName

#include <cstdint>
#include <string_view>

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace net::blocking {

/// @brief Establishes a TCP connection to the given host and port using
/// blocking DNS resolution via @ref net::blocking::GetAddrInfo.
///
/// Resolves the host name with getaddrinfo, then tries to connect to each
/// resolved address in turn. The first successful connection is returned.
/// TCP_NODELAY is set on the socket. Does not use @ref clients::dns::Resolver;
/// for async resolution use @ref net::ConnectTcpByName.
///
/// @param host DNS name or IP address to connect to (e.g. "localhost",
/// "127.0.0.1").
/// @param port TCP port number.
/// @param deadline Maximum time allowed for the whole operation (resolution +
/// connection).
/// @return A connected TCP socket.
/// @throws std::runtime_error if getaddrinfo fails (e.g. unknown host).
/// @throws engine::io::IoException if connection to all resolved
/// addresses fails (e.g. connection refused, timeout).
///
/// @note For async/cached DNS resolution use @ref net::ConnectTcpByName with
/// @ref clients::dns::Resolver.
///
/// @snippet src/net/blocking/connect_tcp_by_name_test.cpp ConnectTcpByName blocking localhost
engine::io::Socket ConnectTcpByName(std::string_view host, std::uint16_t port, engine::Deadline deadline);

}  // namespace net::blocking

USERVER_NAMESPACE_END
