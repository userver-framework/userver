#pragma once

/// @file userver/engine/io/sockaddr.hpp
/// @brief @copybrief engine::io::Sockaddr

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <cstring>
#include <string>

#include <fmt/format.h>
#include <userver/utils/fmt_compat.hpp>

#include <userver/logging/log_helper_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// Socket address-related exceptions
class AddrException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// Multicast request related exceptions
class IpMulticastRequestException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// Communication domain
enum class AddrDomain {
    kUnspecified = AF_UNSPEC,  ///< Unspecified
    kInet = AF_INET,           ///< IPv4
    kInet6 = AF_INET6,         ///< IPv6
    kUnix = AF_UNIX,           ///< Unix socket
};

static_assert(
    AF_UNSPEC == 0,  // NOLINT(misc-redundant-expression)
    "Your socket subsystem looks broken, please contact support chat."
);

/// Native ip multicast request wrapper
class IpMreq final {
public:
    /// @brief Creates IPv4 multicast request.
    /// @param imr_multiaddr IPv4 multicast group address (e.g., "239.255.0.1")
    /// @param imr_interface IPv4 interface address (nullptr for INADDR_ANY)
    IpMreq(const char* imr_multiaddr, const char* imr_interface = nullptr);

    /// @brief Creates IPv6 multicast request.
    /// @param ipv6mr_multiaddr IPv6 multicast group address (e.g., "ff02::1")
    /// @param ipv6mr_interface Interface index (0 for default)
    IpMreq(const char* ipv6mr_multiaddr, unsigned int ipv6mr_interface = 0);

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
    size_t Size() const noexcept { return (family_ == AF_INET ? sizeof(struct ip_mreq) : sizeof(struct ipv6_mreq)); }

private:
    template <typename T>
    T* As() {
        static_assert(sizeof(T) <= sizeof(data_), "Invalid ip multicast request type");
        return reinterpret_cast<T*>(&data_);
    }

    union Storage {
        struct ip_mreq ip_req;
        struct ipv6_mreq ipv6_req;
    } data_;
    int family_;
};

/// Native socket address wrapper
class Sockaddr final {
public:
    /// Constructs an unspecified native socket address.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    Sockaddr() noexcept { ::memset(&data_, 0, sizeof(data_)); }

    /// @brief Wraps a native socket address structure.
    /// @warning sa_family must contain a correct address family.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    explicit Sockaddr(const void* data) {
        const auto* sockaddr = reinterpret_cast<const struct sockaddr*>(data);
        const auto domain = static_cast<AddrDomain>(sockaddr->sa_family);
        ::memcpy(&data_, data, Sockaddr::Addrlen(domain));
    }

    /// @brief Creates address of a Unix socket located at the specified path.
    static Sockaddr MakeUnixSocketAddress(std::string_view path);

    /// @brief Creates the IPv6 loopback address `[::1]:0` that also handles IPv4
    /// connections.
    ///
    /// A program needs to support only this API type to support IPv4 and IPv6.
    static Sockaddr MakeLoopbackAddress() noexcept;

    /// @brief Creates the IPv4 only loopback address `127.0.0.1:0`.
    ///
    /// Prefer a more generic MakeLoopbackAddress() function if not sure.
    static Sockaddr MakeIPv4LoopbackAddress() noexcept;

    /// @brief Domain-specific native socket address structure pointer.
    /// @warning No type checking is performed, user must ensure that only the
    /// correct domain is accessed.
    template <typename T>
    T* As() {
        static_assert(sizeof(T) <= sizeof(data_), "Invalid socket address type");
        return reinterpret_cast<T*>(&data_);
    }

    /// @brief Domain-specific native socket address structure pointer.
    /// @warning No type checking is performed, user must ensure that only the
    /// correct domain is accessed.
    template <typename T>
    const T* As() const {
        static_assert(sizeof(T) <= sizeof(data_), "Invalid socket address type");
        return reinterpret_cast<const T*>(&data_);
    }

    /// Native socket address structure pointer.
    struct sockaddr* Data() { return As<struct sockaddr>(); }

    /// Native socket address structure pointer.
    const struct sockaddr* Data() const { return As<struct sockaddr>(); }

    /// Maximum supported native socket address structure size.
    socklen_t Size() const { return Addrlen(Domain()); }

    /// Maximum supported native socket address structure size.
    socklen_t Capacity() const { return sizeof(data_); }

    /// Protocol family.
    sa_family_t Family() const { return Data()->sa_family; }

    /// Communication domain.
    AddrDomain Domain() const { return static_cast<AddrDomain>(Family()); }

    /// Whether the stored socket address family expects a port.
    bool HasPort() const;

    /// Returns the stored port number if available, otherwise throws.
    std::uint16_t Port() const;

    /// Sets a port for address families that allow for one, otherwise throws.
    void SetPort(std::uint16_t port);

    /// @brief Human-readable address representation.
    /// @note Does not include port number.
    std::string PrimaryAddressString() const;

    /// Domain-specific native socket address structure size.
    static constexpr socklen_t Addrlen(AddrDomain domain) {
        const auto res = AddrlenImpl(domain);

        if (res == 0) {
            throw AddrException(fmt::format("Unexpected address family {}", static_cast<int>(domain)));
        }

        return res;
    }

private:
    static constexpr socklen_t AddrlenImpl(AddrDomain domain) noexcept {
        switch (domain) {
            case AddrDomain::kUnspecified:
                return sizeof(struct sockaddr);
            case AddrDomain::kInet:
                return sizeof(struct sockaddr_in);
            case AddrDomain::kInet6:
                return sizeof(struct sockaddr_in6);
            case AddrDomain::kUnix:
                return sizeof(struct sockaddr_un);
        }

        return 0;
    }

    union Storage {
        struct sockaddr sa_any;
        struct sockaddr_in sa_inet;
        struct sockaddr_in6 sa_inet6;
        struct sockaddr_un sa_unix;
    } data_;
};

/// Outputs human-readable address representation, including port number.
logging::LogHelper& operator<<(logging::LogHelper&, const Sockaddr&);

}  // namespace engine::io

USERVER_NAMESPACE_END

/// Socket address fmt formatter.
template <>
struct fmt::formatter<USERVER_NAMESPACE::engine::io::Sockaddr> {
    static constexpr auto parse(format_parse_context&);

    template <typename FormatContext>
    auto format(const USERVER_NAMESPACE::engine::io::Sockaddr& sa, FormatContext& ctx) USERVER_FMT_CONST;
};

inline constexpr auto fmt::formatter<USERVER_NAMESPACE::engine::io::Sockaddr>::parse(format_parse_context& ctx) {
    const auto* it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
        throw format_error("invalid Sockaddr format");
    }
    return it;
}

template <typename FormatContext>
inline auto fmt::formatter<USERVER_NAMESPACE::engine::io::Sockaddr>::format(
    const USERVER_NAMESPACE::engine::io::Sockaddr& sa,
    FormatContext& ctx
) USERVER_FMT_CONST {
    switch (sa.Domain()) {
        case USERVER_NAMESPACE::engine::io::AddrDomain::kInet:
            return fmt::format_to(ctx.out(), "{}:{}", sa.PrimaryAddressString(), sa.Port());

        case USERVER_NAMESPACE::engine::io::AddrDomain::kInet6:
            return fmt::format_to(ctx.out(), "[{}]:{}", sa.PrimaryAddressString(), sa.Port());

        default:
            return fmt::format_to(ctx.out(), "{}", sa.PrimaryAddressString());
    }
}
