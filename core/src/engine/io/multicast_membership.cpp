#include <userver/engine/io/multicast_membership.hpp>

#include <arpa/inet.h>

#include <fmt/format.h>

#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
IpMreq::IpMreq(const char* ip_multiaddr, unsigned int interface_index) {
    data_.ip_req = {};
    if (inet_pton(AF_INET, ip_multiaddr, &data_.ip_req.imr_multiaddr) == 1) {
        // imr_address field is not set since it's not used if imr_ifindex presents
        family_ = AF_INET;
        data_.ip_req.imr_ifindex = interface_index;
    } else {
        data_.ipv6_req = {};
        if (inet_pton(AF_INET6, ip_multiaddr, &data_.ipv6_req.ipv6mr_multiaddr) == 1) {
            family_ = AF_INET6;
            data_.ipv6_req.ipv6mr_interface = interface_index;
        } else {
            throw IpMulticastRequestException(fmt::format("Invalid IP address: {}", ip_multiaddr));
        }
    }
}

void AddMembership(Socket& socket, const IpMreq& mreq) {
    socket.SetOption(mreq.GetSocketOptionLevel(), mreq.GetJoinSocketOptionName(), mreq.Data(), mreq.Size());
}

void DropMembership(Socket& socket, const IpMreq& mreq) {
    socket.SetOption(mreq.GetSocketOptionLevel(), mreq.GetLeaveSocketOption(), mreq.Data(), mreq.Size());
}

}  // namespace engine::io

USERVER_NAMESPACE_END
