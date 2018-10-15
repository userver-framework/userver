#include <engine/io/addr.hpp>

#include <arpa/inet.h>

#include <array>
#include <iostream>

namespace engine {
namespace io {

std::ostream& operator<<(std::ostream& os, const Addr& addr) {
  switch (addr.Domain()) {
    case AddrDomain::kInet: {
      const auto* inet_addr = addr.As<struct sockaddr_in>();
      std::array<char, INET_ADDRSTRLEN> inet_buf;
      os << inet_ntop(AF_INET, &inet_addr->sin_addr, inet_buf.data(),
                      inet_buf.size())
         << ':' << ntohs(inet_addr->sin_port);
    } break;

    case AddrDomain::kInet6: {
      const auto* inet6_addr = addr.As<struct sockaddr_in6>();
      std::array<char, INET6_ADDRSTRLEN> inet6_buf;
      os << '['
         << inet_ntop(AF_INET6, &inet6_addr->sin6_addr, inet6_buf.data(),
                      inet6_buf.size())
         << "]:" << ntohs(inet6_addr->sin6_port);
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
