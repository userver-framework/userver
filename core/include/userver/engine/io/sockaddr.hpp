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

/// Communication domain
enum class AddrDomain {
  kUnspecified = AF_UNSPEC,  ///< Unspecified
  kInet = AF_INET,           ///< IPv4
  kInet6 = AF_INET6,         ///< IPv6
  kUnix = AF_UNIX,           ///< Unix socket
};

static_assert(
    AF_UNSPEC == 0,
    "Your socket subsystem looks broken, please contact support chat.");

/// Native socket address wrapper
class Sockaddr final {
 public:
  /// Constructs an unspecified native socket address.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  Sockaddr() { ::memset(&data_, 0, sizeof(data_)); }

  /// @brief Wraps a native socket address structure.
  /// @warning sa_family must contain a correct address family.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  explicit Sockaddr(const void* data) {
    const auto* sockaddr = reinterpret_cast<const struct sockaddr*>(data);
    const auto domain = static_cast<AddrDomain>(sockaddr->sa_family);
    ::memcpy(&data_, data, Sockaddr::Addrlen(domain));
  }

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
  int Port() const;

  /// Sets a port for address families that allow for one, otherwise throws.
  void SetPort(int port);

  /// @brief Human-readable address representation.
  /// @note Does not include port number.
  std::string PrimaryAddressString() const;

  /// Domain-specific native socket address structure size.
  static constexpr socklen_t Addrlen(AddrDomain domain) {
    const auto res = AddrlenImpl(domain);

    if (res == 0) {
      throw AddrException(fmt::format("Unexpected address family {}",
                                      static_cast<int>(domain)));
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
  auto format(const USERVER_NAMESPACE::engine::io::Sockaddr& sa,
              FormatContext& ctx) USERVER_FMT_CONST;
};

inline constexpr auto fmt::formatter<
    USERVER_NAMESPACE::engine::io::Sockaddr>::parse(format_parse_context& ctx) {
  const auto* it = ctx.begin();
  if (it != ctx.end() && *it != '}') {
    throw format_error("invalid Sockaddr format");
  }
  return it;
}

template <typename FormatContext>
inline auto fmt::formatter<USERVER_NAMESPACE::engine::io::Sockaddr>::format(
    const USERVER_NAMESPACE::engine::io::Sockaddr& sa,
    FormatContext& ctx) USERVER_FMT_CONST {
  switch (sa.Domain()) {
    case USERVER_NAMESPACE::engine::io::AddrDomain::kInet:
      return fmt::format_to(ctx.out(), "{}:{}", sa.PrimaryAddressString(),
                            sa.Port());

    case USERVER_NAMESPACE::engine::io::AddrDomain::kInet6:
      return fmt::format_to(ctx.out(), "[{}]:{}", sa.PrimaryAddressString(),
                            sa.Port());

    default:
      return fmt::format_to(ctx.out(), "{}", sa.PrimaryAddressString());
  }
}
