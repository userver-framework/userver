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

enum class AddrDomain {
  kInvalid = -1,
  kInet = AF_INET,
  kInet6 = AF_INET6,
  kUnix = AF_UNIX
};

class AddrStorage {
 public:
  AddrStorage() { ::memset(&data_, 0, sizeof(data_)); }

  template <typename T>
  T* As() {
    return reinterpret_cast<T*>(&data_);
  }
  template <typename T>
  const T* As() const {
    return reinterpret_cast<const T*>(&data_);
  }

  struct sockaddr* Data() {
    return As<struct sockaddr>();
  }
  const struct sockaddr* Data() const { return As<struct sockaddr>(); }
  socklen_t Size() const { return sizeof(data_); }

  static constexpr socklen_t Addrlen(AddrDomain domain) {
    switch (domain) {
      case AddrDomain::kInet:
        return sizeof(struct sockaddr_in);
      case AddrDomain::kInet6:
        return sizeof(struct sockaddr_in6);
      case AddrDomain::kUnix:
        return sizeof(struct sockaddr_un);
      case AddrDomain::kInvalid:;
        // fallthrough
    }
    throw std::logic_error("Unexpected address family " +
                           std::to_string(static_cast<int>(domain)));
  }

 private:
  union Storage {
    struct sockaddr any;
    struct sockaddr_in inet;
    struct sockaddr_in6 inet6;
    struct sockaddr_un unix;
  } data_;
};

class Addr {
 public:
  Addr() : domain_(AddrDomain::kInvalid), type_(-1), protocol_(-1) {}

  Addr(const AddrStorage& addr, int type, int protocol)
      : Addr(addr.Data(), type, protocol) {}

  Addr(const void* addr, int type, int protocol)
      : domain_(AddrDomain::kInvalid), type_(type), protocol_(protocol) {
    auto* sockaddr = reinterpret_cast<const struct sockaddr*>(addr);
    domain_ = static_cast<AddrDomain>(sockaddr->sa_family);
    ::memcpy(addr_.Data(), addr, Addrlen());
  }

  AddrDomain Domain() const { return domain_; }
  sa_family_t Family() const { return static_cast<sa_family_t>(domain_); }
  int Type() const { return type_; }
  int Protocol() const { return protocol_; }

  template <typename T>
  const T* As() const {
    return addr_.As<T>();
  }

  const struct sockaddr* Sockaddr() const { return As<struct sockaddr>(); }
  constexpr socklen_t Addrlen() const { return addr_.Addrlen(domain_); }

  std::string RemoteAddress() const;  // without port

 private:
  AddrDomain domain_;
  int type_;
  int protocol_;
  AddrStorage addr_;
};

std::ostream& operator<<(std::ostream&, const Addr&);

}  // namespace io
}  // namespace engine
