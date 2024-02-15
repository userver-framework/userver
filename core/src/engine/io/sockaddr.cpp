#include <userver/engine/io/sockaddr.hpp>

#include <arpa/inet.h>

#include <array>
#include <limits>
#include <ostream>
#include <stdexcept>

#include <fmt/format.h>

#include <userver/engine/io/exception.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

Sockaddr Sockaddr::MakeUnixSocketAddress(std::string_view path) {
  Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_un>();
  sa->sun_family = AF_UNIX;

  static constexpr auto kMaxPathLength = sizeof(sa->sun_path) - 1;
  if (path.size() > kMaxPathLength) {
    throw std::runtime_error(
        fmt::format("unix socket path is too long ({}), max-length={}", path,
                    kMaxPathLength));
  }
  if (path.empty() || path[0] == '\0') {
    throw std::runtime_error("abstract sockets are not supported");
  }
  if (path[0] != '/') {
    throw std::runtime_error(
        fmt::format("unix socket path must be absolute ({})", path));
  }

  std::memcpy(sa->sun_path, path.data(), path.size());
  sa->sun_path[path.size()] = '\0';
  return addr;
}

Sockaddr Sockaddr::MakeLoopbackAddress() noexcept {
  Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr = in6addr_loopback;
  UASSERT(sa->sin6_port == 0);
  return addr;
}

Sockaddr Sockaddr::MakeIPv4LoopbackAddress() noexcept {
  Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_in>();
  sa->sin_family = AF_INET;
  // may be implemented as a macro
  // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
  sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  UASSERT(sa->sin_port == 0);
  return addr;
}

bool Sockaddr::HasPort() const {
  switch (Data()->sa_family) {
    case AF_INET:
    case AF_INET6:
      return true;

    default:
      return false;
  }
}

std::uint16_t Sockaddr::Port() const {
  switch (Data()->sa_family) {
    case AF_INET:
      // may be implemented as a macro
      // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
      return ntohs(As<struct sockaddr_in>()->sin_port);

    case AF_INET6:
      // may be implemented as a macro
      // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
      return ntohs(As<struct sockaddr_in6>()->sin6_port);

    default:
      throw AddrException(fmt::format(
          "Cannot determine port for address family {}", Data()->sa_family));
  }
}

void Sockaddr::SetPort(std::uint16_t port) {
  using PortLimits = std::numeric_limits<in_port_t>;
  if (port < PortLimits::min() || port > PortLimits::max()) {
    throw AddrException(fmt::format("Cannot set invalid port {}", port));
  }
  // may be implemented as a macro
  // NOLINTNEXTLINE(hicpp-no-assembler, readability-isolate-declaration)
  const auto net_port = htons(port);
  switch (Data()->sa_family) {
    case AF_INET:
      As<struct sockaddr_in>()->sin_port = net_port;
      break;
    case AF_INET6:
      As<struct sockaddr_in6>()->sin6_port = net_port;
      break;

    default:
      throw AddrException(
          fmt::format("Unable to set port {} for an unknown address family {}",
                      port, Data()->sa_family));
  }
}

std::string Sockaddr::PrimaryAddressString() const {
  std::array<char, std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)> buf{};

  switch (Domain()) {
    case AddrDomain::kInet: {
      const auto* inet_addr = As<struct sockaddr_in>();
      utils::CheckSyscallNotEqualsCustomException<IoSystemError>(
          ::inet_ntop(AF_INET, &inet_addr->sin_addr, buf.data(), buf.size()),
          nullptr, "converting IPv4 address to string");
      return buf.data();
    } break;

    case AddrDomain::kInet6: {
      const auto* inet6_addr = As<struct sockaddr_in6>();
      utils::CheckSyscallNotEqualsCustomException<IoSystemError>(
          ::inet_ntop(AF_INET6, &inet6_addr->sin6_addr, buf.data(), buf.size()),
          nullptr, "converting IPv6 address to string");
      return buf.data();
    } break;

    case AddrDomain::kUnix:
      return As<struct sockaddr_un>()->sun_path;
      break;

    case AddrDomain::kUnspecified:
      break;
  }
  return "[unknown address]";
}

logging::LogHelper& operator<<(logging::LogHelper& lh, const Sockaddr& sa) {
  return lh << fmt::to_string(sa);
}

}  // namespace engine::io

USERVER_NAMESPACE_END
