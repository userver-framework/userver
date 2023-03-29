#pragma once

/// @file userver/net/blocking/get_addr_info.hpp
/// @brief Blocking Functions for getaddrinfo

#include <vector>

#include <userver/engine/io/sockaddr.hpp>

USERVER_NAMESPACE_BEGIN

namespace net::blocking {

/// @brief Resolve host and port via blocking syscall.
/// @note It is recommended to use clients::dns::Resovler if possible
/// @param host - hostname (or IP) to resolve
/// @param service_and_port - if this argument is a service name, it is
/// translated to the corresponding port number, otherwise treated as port
/// number.
std::vector<engine::io::Sockaddr> GetAddrInfo(std::string_view host,
                                              const char* service_or_port);

}  // namespace net::blocking

USERVER_NAMESPACE_END
