#include <userver/net/connect_tcp_by_name.hpp>

#include <string>
#include <vector>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/net/detail/connect_tcp_to_addrs.hpp>

USERVER_NAMESPACE_BEGIN

namespace net {

engine::io::Socket ConnectTcpByName(
    const std::string& host,
    std::uint16_t port,
    clients::dns::Resolver& resolver,
    engine::Deadline deadline
) {
    auto addrs = resolver.Resolve(host, deadline);
    std::vector<engine::io::Sockaddr> addrs_vec(addrs.begin(), addrs.end());
    for (auto& addr : addrs_vec) {
        addr.SetPort(port);
    }
    return detail::ConnectTcpToAddrs(host, port, addrs_vec, deadline);
}

}  // namespace net

USERVER_NAMESPACE_END
