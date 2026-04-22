#include <userver/net/blocking/connect_tcp_by_name.hpp>

#include <string>

#include <userver/engine/io/sockaddr.hpp>
#include <userver/net/blocking/get_addr_info.hpp>
#include <userver/net/detail/connect_tcp_to_addrs.hpp>

USERVER_NAMESPACE_BEGIN

namespace net::blocking {

engine::io::Socket ConnectTcpByName(std::string_view host, std::uint16_t port, engine::Deadline deadline) {
    const std::string port_str = std::to_string(port);
    auto addrs = GetAddrInfo(host, port_str.c_str());
    return detail::ConnectTcpToAddrs(host, port, addrs, deadline);
}

}  // namespace net::blocking

USERVER_NAMESPACE_END
