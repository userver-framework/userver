#pragma once

/// @file engine/io/addr.hpp
/// @brief @copybrief engine::io::Addr

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <cstring>
#include <iosfwd>
#include <stdexcept>
#include <string>

namespace engine::io {

/// Communication domain
enum class AddrDomain {
  kInvalid = -1,             ///< Invalid
  kUnspecified = AF_UNSPEC,  ///< Unspecified
  kInet = AF_INET,           ///< IPv4
  kInet6 = AF_INET6,         ///< IPv6
  kUnix = AF_UNIX,           ///< Unix socket
};

static_assert(
    AF_UNSPEC == 0,
    "Your socket subsystem looks broken, please contact support chat.");

/// Native socket address wrapper
class AddrStorage final {
 public:
  /// Constructs an unspecified native socket address
  // NOLINTNEXTLINE(hicpp-member-init,cppcoreguidelines-pro-type-member-init)
  AddrStorage() { ::memset(&data_, 0, sizeof(data_)); }

  /// @brief Domain-specific native socket address structure pointer.
  /// @warn No type checking is performed, user must ensure that only the
  /// correct domain is accessed.
  template <typename T>
  T* As() {
    static_assert(sizeof(T) <= sizeof(data_), "Invalid socket address type");
    return reinterpret_cast<T*>(&data_);
  }

  /// @brief Domain-specific native socket address structure pointer.
  /// @warn No type checking is performed, user must ensure that only the
  /// correct domain is accessed.
  template <typename T>
  const T* As() const {
    static_assert(sizeof(T) <= sizeof(data_), "Invalid socket address type");
    return reinterpret_cast<const T*>(&data_);
  }

  /// Native socket address structure pointer.
  struct sockaddr* Data() {
    return As<struct sockaddr>();
  }

  /// Native socket address structure pointer.
  const struct sockaddr* Data() const { return As<struct sockaddr>(); }

  /// Maximum supported native socket address structure size.
  socklen_t Size() const { return sizeof(data_); }

  /// Domain-specific native socket address structure size.
  static constexpr socklen_t Addrlen(AddrDomain domain) {
    switch (domain) {
      case AddrDomain::kUnspecified:
        return sizeof(struct sockaddr);
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

/// Socket address representation
class Addr final {
 public:
  /// Constructs an invalid socket address.
  Addr() = default;

  /// Constructs a socket address from the AddrStorage.
  Addr(const AddrStorage& addr, int type, int protocol)
      : Addr(addr.Data(), type, protocol) {}

  /// @brief Constructs a socket address from the native
  /// socket address structure.
  /// @warn No type checking is performed.
  Addr(const void* addr, int type, int protocol)
      : type_(type), protocol_(protocol) {
    auto* sockaddr = reinterpret_cast<const struct sockaddr*>(addr);
    domain_ = static_cast<AddrDomain>(sockaddr->sa_family);
    ::memcpy(addr_.Data(), addr, Addrlen());
  }

  /// Communication domain.
  AddrDomain Domain() const { return domain_; }

  /// Protocol family.
  sa_family_t Family() const { return static_cast<sa_family_t>(domain_); }

  /// Socket type.
  int Type() const { return type_; }

  /// Protocol.
  /// `0` is accepted for most families (when family has only one protocol).
  int Protocol() const { return protocol_; }

  /// @brief Domain-specific native socket address structure pointer.
  /// @warn No type checking is performed, user must ensure that only the
  /// correct domain is accessed.
  template <typename T>
  const T* As() const {
    return addr_.As<T>();
  }

  /// Native socket address structure pointer.
  const struct sockaddr* Sockaddr() const { return As<struct sockaddr>(); }

  /// Native socket address structure size.
  constexpr socklen_t Addrlen() const {
    return engine::io::AddrStorage::Addrlen(domain_);
  }

  /// @brief Human-readable address representation.
  /// @note Does not include port number.
  std::string RemoteAddress() const;

 private:
  AddrDomain domain_{AddrDomain::kInvalid};
  int type_{-1};
  int protocol_{-1};
  AddrStorage addr_;
};

/// Outputs human-readable address representation, including port number.
std::ostream& operator<<(std::ostream&, const Addr&);

}  // namespace engine::io
