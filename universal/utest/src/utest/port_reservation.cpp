#include <userver/utest/port_reservation.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <algorithm>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <userver/utils/assert.hpp>

#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace {

int BindLoopback(int fd, PortReservation::IpVersion ip_version, std::uint16_t port) {
    if (ip_version == PortReservation::IpVersion::kV4) {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        return ::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    } else {
        sockaddr_in6 address{};
        address.sin6_family = AF_INET6;
        address.sin6_port = htons(port);
        address.sin6_addr = in6addr_loopback;
        return ::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    }
}

void BindFixedPort(int fd, PortReservation::Options options, std::uint16_t port) {
    constexpr std::chrono::milliseconds kRetryInterval{50};
    const auto deadline = std::chrono::steady_clock::now() + options.timeout;
    while (true) {
        const auto result = BindLoopback(fd, options.ip_version, port);
        if (result == 0) {
            return;
        }
        if (errno != EADDRINUSE) {
            utils::CheckSyscall(result, "reserving fixed port {}", port);
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            break;
        }
        std::this_thread::sleep_until(std::min(now + kRetryInterval, deadline));
    }

    throw std::system_error(
        std::make_error_code(std::errc::timed_out),
        fmt::format("Could not reserve fixed port {} within {}: address already in use", port, options.timeout)
    );
}

}  // namespace

PortReservation::PortReservation(Options options) {
    if (options.fixed_port == 0) {
        throw std::invalid_argument("Fixed port must be nonzero");
    }

    const bool is_ipv4 = options.ip_version == IpVersion::kV4;
    const int domain = is_ipv4 ? AF_INET : AF_INET6;
    const int type = options.protocol == Protocol::kTcp ? SOCK_STREAM : SOCK_DGRAM;
    fd_ = fs::blocking::FileDescriptor::AdoptFd(
        utils::CheckSyscall(::socket(domain, type, 0), "creating port reservation socket")
    );
    const auto fd = fd_.GetNative();
    utils::CheckSyscall(::fcntl(fd, F_SETFD, FD_CLOEXEC), "setting close-on-exec on port reservation socket");
    if (options.fixed_port) {
        BindFixedPort(fd, options, *options.fixed_port);
        return;
    }
    utils::CheckSyscall(BindLoopback(fd, options.ip_version, 0), "reserving an ephemeral port");
}

bool PortReservation::IsValid() const noexcept { return fd_.IsOpen(); }

int PortReservation::GetNativeFd() const {
    UINVARIANT(IsValid(), "Port reservation does not own a socket");
    return fd_.GetNative();
}

int PortReservation::ReleaseNativeFd() && {
    UINVARIANT(IsValid(), "Port reservation does not own a socket");
    return std::move(fd_).Release();
}

std::uint16_t PortReservation::GetPort() const {
    sockaddr_storage address{};
    socklen_t size = sizeof(address);
    utils::CheckSyscall(
        ::getsockname(GetNativeFd(), reinterpret_cast<sockaddr*>(&address), &size),
        "reading reserved port"
    );

    if (address.ss_family == AF_INET) {
        return ntohs(reinterpret_cast<const sockaddr_in*>(&address)->sin_port);
    }
    return ntohs(reinterpret_cast<const sockaddr_in6*>(&address)->sin6_port);
}

}  // namespace utest

USERVER_NAMESPACE_END
