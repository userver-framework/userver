#include <engine/io/addr.hpp>

#include <arpa/inet.h>

#include <iostream>
#include <system_error>

#include <boost/algorithm/string/trim.hpp>

namespace engine {
namespace io {

std::string Addr::RemoteAddress() const {
  std::string remote_address;

  switch (Domain()) {
    case AddrDomain::kInet: {
      const auto* inet_addr = As<struct sockaddr_in>();
      remote_address.resize(INET_ADDRSTRLEN);
      if (!inet_ntop(AF_INET, &inet_addr->sin_addr, &remote_address[0],
                     remote_address.size())) {
        throw std::system_error(errno, std::system_category(),
                                "Cannot convert IPv4 address to string");
      }
    } break;

    case AddrDomain::kInet6: {
      const auto* inet6_addr = As<struct sockaddr_in6>();
      remote_address.resize(INET6_ADDRSTRLEN);
      if (!inet_ntop(AF_INET6, &inet6_addr->sin6_addr, &remote_address[0],
                     remote_address.size())) {
        throw std::system_error(errno, std::system_category(),
                                "Cannot convert IPv6 address to string");
      }
    } break;

    case AddrDomain::kUnix:
      remote_address = As<struct sockaddr_un>()->sun_path;
      break;

    default:
      remote_address = "[unknown address]";
  }
  boost::algorithm::trim_right_if(remote_address, [](char c) { return !c; });
  return remote_address;
}

std::ostream& operator<<(std::ostream& os, const Addr& addr) {
  switch (addr.Domain()) {
    case AddrDomain::kInet: {
      const auto* inet_addr = addr.As<struct sockaddr_in>();
      os << addr.RemoteAddress() << ':' << ntohs(inet_addr->sin_port);
    } break;

    case AddrDomain::kInet6: {
      const auto* inet6_addr = addr.As<struct sockaddr_in6>();
      os << '[' << addr.RemoteAddress() << "]:" << ntohs(inet6_addr->sin6_port);
    } break;

    case AddrDomain::kUnix:
      os << addr.As<struct sockaddr_un>()->sun_path;
      break;

    default:
      os << "[unknown address]";
  }
  return os;
}

}  // namespace io
}  // namespace engine
