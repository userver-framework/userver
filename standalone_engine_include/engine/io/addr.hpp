#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <cstring>
#include <iosfwd>
#include <stdexcept>
#include <string>

namespace engine {
namespace io {

enum class AddrDomain { kInet = AF_INET, kInet6 = AF_INET6, kUnix = AF_UNIX };

class Addr {
 public:
  Addr(AddrDomain domain, int type, int protocol, void* addr)
      : domain_(domain), type_(type), protocol_(protocol) {
    auto* sockaddr = reinterpret_cast<struct sockaddr*>(addr);
    if (sockaddr->sa_family != Family()) {
      throw std::logic_error("Address family mismatch: expected " +
                             std::to_string(Family()) + ", got " +
                             std::to_string(sockaddr->sa_family));
    }
    memcpy(&addr_, addr, Addrlen());
  }

  AddrDomain Domain() const { return domain_; }
  sa_family_t Family() const { return static_cast<sa_family_t>(domain_); }
  int Type() const { return type_; }
  int Protocol() const { return protocol_; }

  template <typename T>
  const T* As() const {
    return reinterpret_cast<const T*>(&addr_);
  }

  const struct sockaddr* Sockaddr() const { return As<struct sockaddr>(); }
  constexpr socklen_t Addrlen() const {
    switch (domain_) {
      case AddrDomain::kInet:
        return sizeof(struct sockaddr_in);
      case AddrDomain::kInet6:
        return sizeof(struct sockaddr_in6);
      case AddrDomain::kUnix:
        return sizeof(struct sockaddr_un);
    }
    throw std::logic_error("Unexpected address family " +
                           std::to_string(Family()));
  }

 private:
  AddrDomain domain_;
  int type_;
  int protocol_;
  union {
    struct sockaddr any;
    struct sockaddr_in inet;
    struct sockaddr_in6 inet6;
    struct sockaddr_un unix;
  } addr_;
};

std::ostream& operator<<(std::ostream&, const Addr&);

}  // namespace io
}  // namespace engine
