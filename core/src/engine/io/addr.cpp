#include <engine/io/addr.hpp>

#include <arpa/inet.h>

#include <ostream>

#include <boost/algorithm/string/trim.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <engine/io/exception.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>

namespace engine::io {

std::string Addr::RemoteAddress() const {
  std::string remote_address;

  switch (Domain()) {
    case AddrDomain::kInet: {
      const auto* inet_addr = As<struct sockaddr_in>();
      remote_address.resize(INET_ADDRSTRLEN);
      if (!inet_ntop(AF_INET, &inet_addr->sin_addr, &remote_address[0],
                     remote_address.size())) {
        int err_value = errno;
        throw IoSystemError(err_value)
            << "Cannot convert IPv4 address to string";
      }
    } break;

    case AddrDomain::kInet6: {
      const auto* inet6_addr = As<struct sockaddr_in6>();
      remote_address.resize(INET6_ADDRSTRLEN);
      if (!inet_ntop(AF_INET6, &inet6_addr->sin6_addr, &remote_address[0],
                     remote_address.size())) {
        int err_value = errno;
        throw IoSystemError(err_value)
            << "Cannot convert IPv6 address to string";
      }
    } break;

    case AddrDomain::kUnix:
      remote_address = As<struct sockaddr_un>()->sun_path;
      break;

    case AddrDomain::kInvalid:
      remote_address = "[invalid]";
      break;

    case AddrDomain::kUnspecified:
      remote_address = "[unknown address]";
      break;
  }
  boost::algorithm::trim_right_if(remote_address, [](char c) { return !c; });
  return remote_address;
}

std::string ToString(const Addr& addr) {
  switch (addr.Domain()) {
    case AddrDomain::kInet:
      return fmt::format(FMT_COMPILE("{}:{}"), addr.RemoteAddress(),
                         GetPort(addr));

    case AddrDomain::kInet6:
      return fmt::format(FMT_COMPILE("[{}]:{}"), addr.RemoteAddress(),
                         GetPort(addr));

    default:
      return addr.RemoteAddress();
  }
}

std::ostream& operator<<(std::ostream& os, const Addr& addr) {
  return os << ToString(addr);
}

logging::LogHelper& operator<<(logging::LogHelper& lh, const Addr& addr) {
  return lh << ToString(addr);
}

int GetPort(const Addr& addr) {
  switch (addr.Domain()) {
    case AddrDomain::kInet:
      // NOLINTNEXTLINE(hicpp-no-assembler)
      return ntohs(addr.As<struct sockaddr_in>()->sin_port);

    case AddrDomain::kInet6:
      // NOLINTNEXTLINE(hicpp-no-assembler)
      return ntohs(addr.As<struct sockaddr_in6>()->sin6_port);

    default:
      return -1;
  }
}

}  // namespace engine::io
