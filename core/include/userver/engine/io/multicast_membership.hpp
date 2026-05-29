#pragma once

/// @file userver/engine/io/multicast_membership.hpp
/// @brief @copybrief engine::io::IpMreq

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// Multicast request related exceptions
class IpMulticastRequestException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Socket;

/// Native ip multicast request wrapper
/// @snippet src/engine/io/socket_test.cpp multicast socket creation sample
class IpMreq final {
public:
    /// @brief Creates a structure storing multicast group membership request information. The resulting object may be
    /// passed to @ref engine::io::AddMembership() and @ref engine::io::DropMembership() functions.
    /// @note IP version is chosen automatically from ip_multiaddr value.
    /// @param ip_multiaddr IP multicast group address (e.g. 239.255.0.1" or "ff02::1")
    /// @param interface_index Interface index (0 for default);
    IpMreq(const char* ip_multiaddr, unsigned int interface_index);

    /// @brief Native multicast request structure pointer.
    void* Data() { return &data_; }

    /// @brief Native multicast request structure pointer.
    const void* Data() const { return &data_; }

    /// @brief Returns socket option level.
    int GetSocketOptionLevel() const noexcept { return (family_ == AF_INET ? IPPROTO_IP : IPPROTO_IPV6); }

    /// @brief Returns socket option name for joining multicast group.
    int GetJoinSocketOptionName() const noexcept { return (family_ == AF_INET ? IP_ADD_MEMBERSHIP : IPV6_JOIN_GROUP); }

    /// @brief Returns socket option name for leaving multicast group.
    int GetLeaveSocketOption() const noexcept { return (family_ == AF_INET ? IP_DROP_MEMBERSHIP : IPV6_LEAVE_GROUP); }

    /// Returns appropriate size for setsockopt based on address family.
    /// @param domain Socket domain (AF_INET or AF_INET6)
    size_t Size() const noexcept { return (family_ == AF_INET ? sizeof(struct ip_mreqn) : sizeof(struct ipv6_mreq)); }

private:
    union Storage {
        struct ip_mreqn ip_req;
        struct ipv6_mreq ipv6_req;
    } data_;
    int family_;
};

/// @brief Joins multicast group for given socket to receive multicast datagrams.
void AddMembership(Socket& socket, const IpMreq& mreq);

/// @brief Leaves multicast group for given socket previously joined with AddMembership.
void DropMembership(Socket& socket, const IpMreq& mreq);

}  // namespace engine::io

USERVER_NAMESPACE_END
